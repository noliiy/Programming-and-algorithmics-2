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
#include <array>
#include <string>

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


// helper to build a nested initializer_list type of depth N
template <typename T, size_t N>
struct CNested
{
    // recursive case: list of (depth N-1) lists
    using type = std::initializer_list<typename CNested<T, N - 1>::type>;
};

template <typename T>
struct CNested<T, 1>
{
    // base case: a flat list of T
    using type = std::initializer_list<T>;
};

// short alias so the constructor signature stays readable
template <typename T, size_t N>
using CNestedT = typename CNested<T, N>::type;


// describes one axis of a slice (fixed value, forward range, or reversed range)
struct CSlicePart
{
    bool    m_Fixed    = false; // true when the axis was fixed by an integer arg
    size_t  m_FixedVal = 0;     // the fixed integer (only valid if m_Fixed)
    size_t  m_From     = 0;     // first endpoint of the range
    size_t  m_To       = 0;     // second endpoint of the range
    bool    m_Reversed = false; // true if the range came from CRangeRev

    // visible length of this axis inside the view
    size_t length () const
    {
        // reversed range goes high to low so length = from - to + 1
        return m_Reversed ? ( m_From - m_To + 1 ) : ( m_To - m_From + 1 );
    }

    // map a view-local index back to the index in the original tensor
    size_t map ( size_t i ) const
    {
        // walk forward from m_From, or backward when reversed
        return m_Reversed ? ( m_From - i ) : ( m_From + i );
    }
};


// forward declaration so CTensor can return CTensorView from slice
template <typename T_, size_t DIM_>
class CTensorView;


template <typename T_, size_t DIM_>
class CTensor
{
  public:
    // size constructor: first arg is the fill value, then DIM_ axis sizes
    template <typename ... S>
    CTensor ( const T_ & val, S ... sizes )
    {
        // arg count must equal DIM_, otherwise we cannot fill m_Sizes
        static_assert ( sizeof...(S) == DIM_, "wrong number of sizes" );
        // turn the parameter pack into a small temp array
        size_t tmp [] = { static_cast<size_t> ( sizes ) ... };
        // total cell count = product of all sizes
        size_t total = 1;
        // copy sizes into the member array and multiply for the total
        for ( size_t i = 0; i < DIM_; ++ i )
        {
            m_Sizes [ i ] = tmp [ i ];
            total *= tmp [ i ];
        }
        // fill the storage with the given value
        m_Data . assign ( total, val );
    }

    // initializer_list constructor for any DIM_ (uses the recursive nested type)
    CTensor ( CNestedT<T_, DIM_> il )
    {
        // first walk: figure out how big each axis is from the leftmost path
        deriveSize<0> ( il );
        // second walk: copy values and verify every nested list has the right length
        appendData<0> ( il );
    }

    // element access: parameter count must equal DIM_ (compile-time check)
    template <typename ... I>
    T_ & operator () ( I ... idx )
    {
        // exact match required, otherwise the index pack does not fill m_Sizes
        static_assert ( sizeof...(I) == DIM_, "wrong number of indices" );
        // pack indices into a fixed array
        std::array<size_t, DIM_> ix { static_cast<size_t> ( idx ) ... };
        // bounds check per axis
        for ( size_t i = 0; i < DIM_; ++ i )
            if ( ix [ i ] >= m_Sizes [ i ] )
                throw std::out_of_range ( "tensor index out of range" );
        // resolve the linear offset and return the cell
        return m_Data [ linear ( ix ) ];
    }

    // const version of the same access
    template <typename ... I>
    const T_ & operator () ( I ... idx ) const
    {
        static_assert ( sizeof...(I) == DIM_, "wrong number of indices" );
        std::array<size_t, DIM_> ix { static_cast<size_t> ( idx ) ... };
        for ( size_t i = 0; i < DIM_; ++ i )
            if ( ix [ i ] >= m_Sizes [ i ] )
                throw std::out_of_range ( "tensor index out of range" );
        return m_Data [ linear ( ix ) ];
    }

    // slice returns a view whose dimension is figured out at compile time
    template <typename ... A>
    auto slice ( A ... args )
    {
        // cannot slice with more args than the tensor has axes
        static_assert ( sizeof...(A) <= DIM_, "too many slice arguments" );
        // count args that are plain integers (those collapse the axis)
        constexpr size_t fixedCount =
            ( ( std::is_integral<typename std::decay<A>::type>::value ? size_t ( 1 ) : size_t ( 0 ) )
              + ... + size_t ( 0 ) );
        // free args = args that survive (CRange / CRangeRev)
        constexpr size_t freeCount = sizeof...(A) - fixedCount;
        // missing trailing axes always survive as full ranges
        constexpr size_t newDim = freeCount + ( DIM_ - sizeof...(A) );

        // one CSlicePart per axis of this tensor
        std::vector<CSlicePart> parts ( DIM_ );
        // fill the leading parts from the args
        if constexpr ( sizeof...(A) > 0 )
            fillParts<0> ( parts, args ... );
        // pad the trailing axes with the full range from the tensor
        for ( size_t i = sizeof...(A); i < DIM_; ++ i )
        {
            parts [ i ] . m_Fixed    = false;
            parts [ i ] . m_From     = 0;
            parts [ i ] . m_To       = m_Sizes [ i ] ? m_Sizes [ i ] - 1 : 0;
            parts [ i ] . m_Reversed = false;
        }
        // every part has to fit inside the actual axis size
        for ( size_t i = 0; i < DIM_; ++ i )
        {
            if ( parts [ i ] . m_Fixed )
            {
                if ( parts [ i ] . m_FixedVal >= m_Sizes [ i ] )
                    throw std::out_of_range ( "slice out of range" );
            }
            else
            {
                size_t hi = parts [ i ] . m_Reversed ? parts [ i ] . m_From : parts [ i ] . m_To;
                if ( hi >= m_Sizes [ i ] )
                    throw std::out_of_range ( "slice out of range" );
            }
        }
        // capture this tensor and build a type-erased read/write callback
        auto self = this;
        auto acc = [ self ] ( const std::vector<size_t> & mapped ) -> T_ &
        {
            // copy back into a fixed-size array so we can call operator() with a pack
            std::array<size_t, DIM_> arr;
            for ( size_t i = 0; i < DIM_; ++ i )
                arr [ i ] = mapped [ i ];
            // unpack the array straight into operator() via std::apply
            return std::apply (
                [ self ] ( auto ... a ) -> T_ & { return ( * self ) ( a ... ); },
                arr );
        };
        // hand the callback and the slice description over to the view
        return CTensorView<T_, newDim> ( std::move ( acc ), DIM_, std::move ( parts ) );
    }

    // pretty print using the same brace layout as in the test cases
    friend std::ostream & operator << ( std::ostream & os, const CTensor & t )
    {
        // walking idx that gets filled axis by axis
        std::array<size_t, DIM_> idx {};
        // recursive printer starts at axis 0 with an empty indent prefix
        t . printAxis ( os, 0, idx, "" );
        return os;
    }

  private:
    // figure out the axis sizes by following the first element of every nested list
    template <size_t Axis, typename L>
    void deriveSize ( const L & l )
    {
        // size at this depth equals the list size at this depth
        m_Sizes [ Axis ] = l . size ();
        if constexpr ( Axis + 1 < DIM_ )
        {
            // dive deeper through the first sublist if there is one
            if ( l . size () > 0 )
                deriveSize<Axis + 1> ( * l . begin () );
            else
                // empty list -> rest of the axes default to zero
                for ( size_t k = Axis + 1; k < DIM_; ++ k )
                    m_Sizes [ k ] = 0;
        }
    }

    // walk every nested list, verify its length, and copy leaf values into m_Data
    template <size_t Axis, typename L>
    void appendData ( const L & l )
    {
        // every list at this depth must match m_Sizes[Axis]
        if ( l . size () != m_Sizes [ Axis ] )
            throw std::invalid_argument ( "tensor is not a hyper-prism" );
        if constexpr ( Axis + 1 < DIM_ )
        {
            // recurse into each sublist
            for ( const auto & sub : l )
                appendData<Axis + 1> ( sub );
        }
        else
        {
            // leaves: copy the values out one by one
            for ( const auto & x : l )
                m_Data . push_back ( x );
        }
    }

    // turn an N-d index into a flat row-major offset
    size_t linear ( const std::array<size_t, DIM_> & ix ) const
    {
        size_t off = 0;
        // standard row-major formula: stride of axis d = product of sizes after d
        for ( size_t d = 0; d < DIM_; ++ d )
        {
            size_t stride = 1;
            for ( size_t k = d + 1; k < DIM_; ++ k )
                stride *= m_Sizes [ k ];
            off += ix [ d ] * stride;
        }
        return off;
    }

    // parse one slice arg into parts[I] then recurse over the rest
    template <size_t I, typename F, typename ... R>
    void fillParts ( std::vector<CSlicePart> & parts, F f, R ... rest )
    {
        if constexpr ( std::is_integral_v<std::decay_t<F>> )
        {
            // integer means this axis is collapsed to a fixed value
            parts [ I ] . m_Fixed    = true;
            parts [ I ] . m_FixedVal = static_cast<size_t> ( f );
        }
        else if constexpr ( std::is_same_v<std::decay_t<F>, CRange> )
        {
            parts [ I ] . m_Fixed    = false;
            parts [ I ] . m_From     = f . m_From;
            parts [ I ] . m_To       = f . m_To;
            parts [ I ] . m_Reversed = false;
        }
        else if constexpr ( std::is_same_v<std::decay_t<F>, CRangeRev> )
        {
            parts [ I ] . m_Fixed    = false;
            parts [ I ] . m_From     = f . m_From;
            parts [ I ] . m_To       = f . m_To;
            parts [ I ] . m_Reversed = true;
        }
        // pop one off the front and keep going
        if constexpr ( sizeof...(rest) > 0 )
            fillParts<I + 1> ( parts, rest ... );
    }

    // recursive printer used by operator<<
    void printAxis ( std::ostream & os,
                     size_t axis,
                     std::array<size_t, DIM_> & idx,
                     const std::string & indent ) const
    {
        if ( axis + 1 == DIM_ )
        {
            // innermost axis: single line, comma separated, in braces
            os << "{";
            for ( size_t i = 0; i < m_Sizes [ axis ]; ++ i )
            {
                if ( i ) os << ", ";
                idx [ axis ] = i;
                os << m_Data [ linear ( idx ) ];
            }
            os << "}";
        }
        else
        {
            // outer axes: multi-line with indented sub-tensors
            os << "{\n";
            for ( size_t i = 0; i < m_Sizes [ axis ]; ++ i )
            {
                idx [ axis ] = i;
                // each sub-tensor gets two more spaces of indent
                os << indent << "  ";
                printAxis ( os, axis + 1, idx, indent + "  " );
                if ( i + 1 < m_Sizes [ axis ] ) os << ",";
                os << "\n";
            }
            // closing brace lines up with the opening level
            os << indent << "}";
        }
    }

    // size of every axis (compile-time fixed length)
    std::array<size_t, DIM_>    m_Sizes;
    // flat row-major storage
    std::vector<T_>             m_Data;
};


template <typename T_, size_t DIM_>
class CTensorView
{
  public:
    // type-erased read/write callback into the original tensor
    using Accessor = std::function < T_ & ( const std::vector<size_t> & ) >;

    // store the callback, the original tensor's rank, and the slice description
    CTensorView ( Accessor acc,
                  size_t origDim,
                  std::vector<CSlicePart> parts )
        : m_Acc      ( std::move ( acc ) ),
          m_OrigDim  ( origDim ),
          m_Parts    ( std::move ( parts ) )
    {
        // pre-compute the lengths of the free (non-fixed) axes for printing
        for ( const auto & p : m_Parts )
            if ( ! p . m_Fixed )
                m_FreeLens . push_back ( p . length () );
    }

    // operator() with strict arg count check
    template <typename ... I>
    T_ & operator () ( I ... idx ) const
    {
        // view's operator() must receive exactly DIM_ indices
        static_assert ( sizeof...(I) == DIM_, "wrong number of indices for view" );
        // pack the indices (zero-sized array is fine when DIM_ == 0)
        std::array<size_t, DIM_> user { static_cast<size_t> ( idx ) ... };
        // delegate to the array-based implementation
        return at ( user );
    }

    // print uses the same brace layout as CTensor
    friend std::ostream & operator << ( std::ostream & os, const CTensorView & v )
    {
        if constexpr ( DIM_ == 0 )
        {
            // 0-D view is just the element itself
            os << v ();
        }
        else
        {
            std::array<size_t, DIM_> idx {};
            v . printAxis ( os, 0, idx, "" );
        }
        return os;
    }

  private:
    // resolve a view-local index into the actual element via the accessor
    T_ & at ( const std::array<size_t, DIM_> & user ) const
    {
        // build the full original-tensor index dim by dim
        std::vector<size_t> mapped ( m_OrigDim );
        // counter for which user index we have consumed so far
        size_t u = 0;
        for ( size_t d = 0; d < m_OrigDim; ++ d )
        {
            if ( m_Parts [ d ] . m_Fixed )
            {
                // collapsed axis: the original index is fixed
                mapped [ d ] = m_Parts [ d ] . m_FixedVal;
            }
            else
            {
                // free axis: take the next user index, check, and map it
                if constexpr ( DIM_ > 0 )
                {
                    if ( user [ u ] >= m_Parts [ d ] . length () )
                        throw std::out_of_range ( "view index out of range" );
                    mapped [ d ] = m_Parts [ d ] . map ( user [ u ] );
                    ++ u;
                }
                else
                {
                    // should not happen: 0-D view has no free axes by construction
                    throw std::out_of_range ( "view index missing" );
                }
            }
        }
        return m_Acc ( mapped );
    }

    // recursive printer mirroring CTensor::printAxis but using free axis lengths
    void printAxis ( std::ostream & os,
                     size_t axis,
                     std::array<size_t, DIM_> & idx,
                     const std::string & indent ) const
    {
        size_t len = m_FreeLens [ axis ];
        if ( axis + 1 == DIM_ )
        {
            // innermost axis: one line of values
            os << "{";
            for ( size_t i = 0; i < len; ++ i )
            {
                if ( i ) os << ", ";
                idx [ axis ] = i;
                os << at ( idx );
            }
            os << "}";
        }
        else
        {
            // outer axis: indented multi-line block
            os << "{\n";
            for ( size_t i = 0; i < len; ++ i )
            {
                idx [ axis ] = i;
                os << indent << "  ";
                printAxis ( os, axis + 1, idx, indent + "  " );
                if ( i + 1 < len ) os << ",";
                os << "\n";
            }
            os << indent << "}";
        }
    }

    // accessor into the backing tensor (type-erased)
    Accessor                    m_Acc;
    // dimension of the original tensor (>= DIM_)
    size_t                      m_OrigDim;
    // one slice part per original axis
    std::vector<CSlicePart>     m_Parts;
    // length of each free axis, used by the printer
    std::vector<size_t>         m_FreeLens;
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
