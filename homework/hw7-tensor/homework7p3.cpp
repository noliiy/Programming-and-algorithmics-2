/**
 * PA2 Homework 7 (part 3) - generic CTensor
 * 
 * Final variant: arbitrary-dimension CTensor with views on views,
 * compile-time dimension tracking and variadic-template slicing.
 */

#ifndef __PROGTEST__
#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <memory>
#include <optional>
#include <iterator>
#include <functional>
#include <stdexcept>
#include <type_traits>
#include <cassert>

class CRange
{
  public:
                                       CRange                                  ( size_t                                from,
                                                                                 size_t                                to  )
      : m_From ( from ),
        m_To  ( to )
    {
      if ( from > to )
        throw std::invalid_argument ( "invalid range" );
    }
    size_t                             m_From;
    size_t                             m_To;
};

class CRangeRev
{
  public:
                                       CRangeRev                               ( size_t                                from,
                                                                                 size_t                                to  )
      : m_From ( from ),
        m_To  ( to )
    {
      if ( from < to )
        throw std::invalid_argument ( "invalid range" );
    }
    size_t                             m_From;
    size_t                             m_To;
};
#endif /* __PROGTEST__ */


// small struct that describes what happens in one dimension of a slice
struct CSlicePart
{
    bool    m_Fixed    = false; // true when the user passed a single number for this dim
    size_t  m_FixedVal = 0;     // the fixed number (only valid when m_Fixed is true)
    size_t  m_From     = 0;     // start of the range (CRange / CRangeRev)
    size_t  m_To       = 0;     // end of the range
    bool    m_Reversed = false; // true if it came from CRangeRev

    // how many elements this dim contributes to the view
    size_t length () const
    {
        // for a reversed range from is the bigger one, otherwise to is bigger
        return m_Reversed ? ( m_From - m_To + 1 ) : ( m_To - m_From + 1 );
    }

    // turn a view index into the original tensor index
    size_t map ( size_t i ) const
    {
        // if reversed, we go down from m_From, otherwise up from m_From
        return m_Reversed ? ( m_From - i ) : ( m_From + i );
    }
};

// forward declaration so CTensor::slice can return a CTensorView
template <typename T_>
class CTensorView;

template <typename T_, size_t DIM_>
class CTensor
{
  public:
    // build a tensor of the given size and fill every cell with val
    template <typename ... S>
    CTensor ( const T_ & val, S ... sizes )
    {
        // make sure the user passed exactly DIM_ size arguments
        static_assert ( sizeof...(sizes) == DIM_, "wrong number of sizes" );
        // pack the parameter pack into a small temporary array
        size_t tmp [] = { static_cast<size_t> ( sizes ) ... };
        // total number of elements in the data buffer
        size_t total = 1;
        // copy the sizes into the member array and compute the product
        for ( size_t i = 0; i < DIM_; ++ i )
        {
            m_Sizes [ i ] = tmp [ i ];
            total *= tmp [ i ];
        }
        // allocate the buffer and fill every cell with the given value
        m_Data . assign ( total, val );
    }

    // initializer_list constructor for the 1D case
    CTensor ( std::initializer_list<T_> il ) requires ( DIM_ == 1 )
    {
        // size of the only dimension is the list size
        m_Sizes [ 0 ] = il . size ();
        // copy the values directly into the buffer
        m_Data . assign ( il . begin (), il . end () );
    }

    // initializer_list constructor for the 2D case
    CTensor ( std::initializer_list<std::initializer_list<T_>> il ) requires ( DIM_ == 2 )
    {
        // outer list size is the number of rows
        m_Sizes [ 0 ] = il . size ();
        // inner size is taken from the first row, 0 if there is no row
        m_Sizes [ 1 ] = il . size () ? il . begin () -> size () : 0;
        // walk every row and copy it after checking the shape
        for ( const auto & row : il )
        {
            // every row must have the same length, otherwise it's not a hyper-prism
            if ( row . size () != m_Sizes [ 1 ] )
                throw std::invalid_argument ( "tensor is not a hyper-prism" );
            // append the row's values to the linear buffer
            for ( const auto & x : row )
                m_Data . push_back ( x );
        }
    }

    // element access for 1D tensor (mutable)
    T_ & operator () ( size_t i ) requires ( DIM_ == 1 )
    {
        // simple bounds check
        if ( i >= m_Sizes [ 0 ] )
            throw std::out_of_range ( "tensor index out of range" );
        // direct access into the flat buffer
        return m_Data [ i ];
    }

    // element access for 1D tensor (const version)
    const T_ & operator () ( size_t i ) const requires ( DIM_ == 1 )
    {
        if ( i >= m_Sizes [ 0 ] )
            throw std::out_of_range ( "tensor index out of range" );
        return m_Data [ i ];
    }

    // element access for 2D tensor (mutable)
    T_ & operator () ( size_t i, size_t j ) requires ( DIM_ == 2 )
    {
        // both indices must be inside the size
        if ( i >= m_Sizes [ 0 ] || j >= m_Sizes [ 1 ] )
            throw std::out_of_range ( "tensor index out of range" );
        // row-major mapping into the flat buffer
        return m_Data [ i * m_Sizes [ 1 ] + j ];
    }

    // element access for 2D tensor (const version)
    const T_ & operator () ( size_t i, size_t j ) const requires ( DIM_ == 2 )
    {
        if ( i >= m_Sizes [ 0 ] || j >= m_Sizes [ 1 ] )
            throw std::out_of_range ( "tensor index out of range" );
        return m_Data [ i * m_Sizes [ 1 ] + j ];
    }

    // build a view from the given slice arguments
    template <typename ... A>
    CTensorView<T_> slice ( A ... args )
    {
        // we accept at most DIM_ arguments (rest is filled with full ranges)
        static_assert ( sizeof...(args) <= DIM_, "too many slice arguments" );
        // one CSlicePart per dimension of this tensor
        std::vector<CSlicePart> parts ( DIM_ );
        // fill the first sizeof...(args) entries from the parameter pack
        if constexpr ( sizeof...(args) > 0 )
            fillParts<0> ( parts, args ... );
        // remaining trailing dimensions are unrestricted (full range)
        for ( size_t i = sizeof...(args); i < DIM_; ++ i )
        {
            parts [ i ] . m_Fixed    = false;
            parts [ i ] . m_From     = 0;
            parts [ i ] . m_To       = m_Sizes [ i ] ? m_Sizes [ i ] - 1 : 0;
            parts [ i ] . m_Reversed = false;
        }
        // every part has to fit inside the actual tensor size
        for ( size_t i = 0; i < DIM_; ++ i )
        {
            if ( parts [ i ] . m_Fixed )
            {
                // fixed value cannot reach beyond the size
                if ( parts [ i ] . m_FixedVal >= m_Sizes [ i ] )
                    throw std::out_of_range ( "slice out of range" );
            }
            else
            {
                // for ranges, the largest used index is m_From if reversed, else m_To
                size_t hi = parts [ i ] . m_Reversed ? parts [ i ] . m_From : parts [ i ] . m_To;
                if ( hi >= m_Sizes [ i ] )
                    throw std::out_of_range ( "slice out of range" );
            }
        }
        // capture the tensor pointer so the view can read/write the data later
        auto self = this;
        // build a small lambda that knows how to access this exact tensor
        auto acc = [ self ] ( const std::vector<size_t> & idx ) -> T_ &
        {
            // dispatch on the dimension at compile time
            if constexpr ( DIM_ == 1 )
                return ( * self ) ( idx [ 0 ] );
            else
                return ( * self ) ( idx [ 0 ], idx [ 1 ] );
        };
        // hand everything to the view and we are done
        return CTensorView<T_> ( acc, DIM_, std::move ( parts ) );
    }

    // print the tensor in the format used by the test cases
    friend std::ostream & operator << ( std::ostream & os, const CTensor & t )
    {
        if constexpr ( DIM_ == 1 )
        {
            // 1D: a single line, e.g. {1, 2, 3}
            os << "{";
            for ( size_t i = 0; i < t . m_Sizes [ 0 ]; ++ i )
            {
                if ( i ) os << ", ";
                os << t . m_Data [ i ];
            }
            os << "}";
        }
        else
        {
            // 2D: outer braces on their own line, each row indented with two spaces
            os << "{\n";
            for ( size_t i = 0; i < t . m_Sizes [ 0 ]; ++ i )
            {
                os << "  {";
                for ( size_t j = 0; j < t . m_Sizes [ 1 ]; ++ j )
                {
                    if ( j ) os << ", ";
                    os << t . m_Data [ i * t . m_Sizes [ 1 ] + j ];
                }
                os << "}";
                // every row except the last gets a trailing comma
                if ( i + 1 < t . m_Sizes [ 0 ] ) os << ",";
                os << "\n";
            }
            os << "}";
        }
        return os;
    }

  private:
    // little recursive helper that fills parts[I] from the next argument
    template <size_t I, typename F, typename ... R>
    void fillParts ( std::vector<CSlicePart> & parts, F f, R ... rest )
    {
        // a plain integer means a fixed index
        if constexpr ( std::is_integral_v<F> )
        {
            parts [ I ] . m_Fixed    = true;
            parts [ I ] . m_FixedVal = static_cast<size_t> ( f );
        }
        // a CRange is a forward range
        else if constexpr ( std::is_same_v<F, CRange> )
        {
            parts [ I ] . m_Fixed    = false;
            parts [ I ] . m_From     = f . m_From;
            parts [ I ] . m_To       = f . m_To;
            parts [ I ] . m_Reversed = false;
        }
        // a CRangeRev is the same but walked backwards
        else if constexpr ( std::is_same_v<F, CRangeRev> )
        {
            parts [ I ] . m_Fixed    = false;
            parts [ I ] . m_From     = f . m_From;
            parts [ I ] . m_To       = f . m_To;
            parts [ I ] . m_Reversed = true;
        }
        // recurse into the rest of the pack if there is anything left
        if constexpr ( sizeof...(rest) > 0 )
            fillParts<I + 1> ( parts, rest ... );
    }

    // sizes of every dimension
    std::array<size_t, DIM_>    m_Sizes;
    // flat row-major storage of the tensor values
    std::vector<T_>             m_Data;
};


template <typename T_>
class CTensorView
{
  public:
    // function type that knows how to read one element of the original tensor
    using Accessor = std::function < T_ & ( const std::vector<size_t> & ) >;

    // we keep the accessor, the original dimension and one CSlicePart per original dim
    CTensorView ( Accessor acc, size_t origDim, std::vector<CSlicePart> parts )
        : m_Acc      ( std::move ( acc ) ),
          m_OrigDim  ( origDim ),
          m_Parts    ( std::move ( parts ) )
    {
    }

    // generic call operator -- works for 1 or 2 view indices
    template <typename ... I>
    T_ & operator () ( I ... idx ) const
    {
        // pack the user's indices into a small array
        std::array<size_t, sizeof...(idx)> user { static_cast<size_t> ( idx ) ... };
        // we will fill the index for the original tensor here
        std::vector<size_t> mapped ( m_OrigDim );
        // counts how many user indices we have already consumed
        size_t f = 0;
        // walk every dimension of the original tensor
        for ( size_t i = 0; i < m_OrigDim; ++ i )
        {
            if ( m_Parts [ i ] . m_Fixed )
            {
                // fixed dim, no user index needed
                mapped [ i ] = m_Parts [ i ] . m_FixedVal;
            }
            else
            {
                // we need one more user index for this dim
                if ( f >= user . size () )
                    throw std::out_of_range ( "not enough indices" );
                size_t u = user [ f ++ ];
                // user index has to be inside the view length
                if ( u >= m_Parts [ i ] . length () )
                    throw std::out_of_range ( "view index out of range" );
                // map it back to the original tensor index
                mapped [ i ] = m_Parts [ i ] . map ( u );
            }
        }
        // if there are user indices left, the user passed too many
        if ( f != user . size () )
            throw std::out_of_range ( "too many indices" );
        // finally read/write the element through the captured accessor
        return m_Acc ( mapped );
    }

    // print the view -- the layout is the same as for CTensor
    friend std::ostream & operator << ( std::ostream & os, const CTensorView & v )
    {
        // collect the lengths of the free (non-fixed) dimensions
        std::vector<size_t> freeSz;
        for ( const auto & p : v . m_Parts )
            if ( ! p . m_Fixed )
                freeSz . push_back ( p . length () );
        // 1D view: single line of values
        if ( freeSz . size () == 1 )
        {
            os << "{";
            for ( size_t i = 0; i < freeSz [ 0 ]; ++ i )
            {
                if ( i ) os << ", ";
                os << v ( i );
            }
            os << "}";
        }
        else
        {
            // 2D view: same shape as 2D CTensor printing
            os << "{\n";
            for ( size_t i = 0; i < freeSz [ 0 ]; ++ i )
            {
                os << "  {";
                for ( size_t j = 0; j < freeSz [ 1 ]; ++ j )
                {
                    if ( j ) os << ", ";
                    os << v ( i, j );
                }
                os << "}";
                if ( i + 1 < freeSz [ 0 ] ) os << ",";
                os << "\n";
            }
            os << "}";
        }
        return os;
    }

  private:
    // accessor lambda captured from the original tensor
    Accessor                    m_Acc;
    // dimension of the original tensor (1 or 2)
    size_t                      m_OrigDim;
    // one CSlicePart per dimension of the original tensor
    std::vector<CSlicePart>     m_Parts;
};


#ifndef __PROGTEST__
template <typename T_>
std::string  toString ( const T_ & x )
{
  std::ostringstream oss;
  oss << x;
  return oss . str ();
}

int main ()
{
  CTensor<int,2> m1 ( 0, 3, 5 );
  CTensor<int,2> m2 { { 1, 2, 3 }, { 4, 5, 6 } };
  CTensor<std::string,2> m4
  {
    { "test", "progtest" },
    { "PA1", "PA2" }
  };

  assert ( toString ( m1 ) == R"({
  {0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0}
})" );

  assert ( toString ( m2 ) == R"({
  {1, 2, 3},
  {4, 5, 6}
})" );

  assert ( toString ( m4 ) == R"({
  {test, progtest},
  {PA1, PA2}
})" );

  assert ( toString ( m2 . slice ( CRange ( 0, 1 ), CRange ( 1, 2 ) ) ) == R"({
  {2, 3},
  {5, 6}
})" );
  assert ( toString ( m2 . slice ( 0, CRange ( 1, 2 ) ) ) == "{2, 3}" );
  assert ( toString ( m2 . slice ( 1 ) ) == "{4, 5, 6}" );
  assert ( toString ( m2 . slice ( CRange ( 0, 1 ), 2 ) ) == "{3, 6}" );

  auto v2 = m2 . slice ( CRange ( 0, 1 ), CRange ( 1, 2 ) );
  assert ( v2 ( 0, 0 ) == 2 );
  v2 ( 1, 1 ) = 666;
  assert ( v2 ( 1, 1 ) == 666 );

  assert ( toString ( v2 ) == R"({
  {2, 3},
  {5, 666}
})" );

  assert ( toString ( m2 ) == R"({
  {1, 2, 3},
  {4, 5, 666}
})" );

  assert ( toString ( m2 . slice ( 1, CRange ( 1, 2 ) ) ) == "{5, 666}" );
  assert ( toString ( m2 . slice ( 1, CRangeRev ( 2, 1 ) ) ) == "{666, 5}" );
  assert ( toString ( m2 . slice ( 1, CRange ( 2, 2 ) ) ) == "{666}" );
  assert ( toString ( m2 . slice ( CRange ( 1, 1 ), 2 ) ) == "{666}" );
  assert ( toString ( m2 . slice ( CRange ( 1, 1 ), CRange ( 2, 2 ) ) ) == R"###({
  {666}
})###" );

  assert ( toString ( m4 . slice ( CRangeRev ( 1, 0 ), CRangeRev ( 1, 0 ) ) ) == R"({
  {PA2, PA1},
  {progtest, test}
})" );

  try
  {
    m2 ( 3, 4 ) ++;
    assert ( "missing an exception" == nullptr );
  }
  catch ( const std::exception & e )
  {
  }

  try
  {
    v2 ( 2, 0 ) --;
    assert ( "missing an exception" == nullptr );
  }
  catch ( const std::exception & e )
  {
  }

  try
  {
    CTensor<int, 2> m5
    {
      { 3, 5, 8 },
      { 2, 7 }
    };
    assert ( "missing an exception" == nullptr );
  }
  catch ( const std::exception & e )
  {
  }

  try
  {
    m1 . slice ( CRange ( 0, 4 ), CRange ( 0, 2 ) );
    assert ( "missing an exception" == nullptr );
  }
  catch ( const std::exception & e )
  {
  }

  return EXIT_SUCCESS;
}
#endif /* __PROGTEST__ */
