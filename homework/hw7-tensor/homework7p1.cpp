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

// one dimension of a slice: fixed index, forward range, or reversed range
struct CSlicePart
{
    bool    m_Fixed    = false; // set when the arg was a plain number
    size_t  m_FixedVal = 0;     // that number (only if m_Fixed)
    size_t  m_From     = 0;     // range low / high side depending on m_Reversed
    size_t  m_To       = 0;     // other end of the range
    bool    m_Reversed = false; // true for CRangeRev

    // how long this axis is inside the view (if not fixed)
    size_t length () const
    {
        // reversed runs from high to low so length is from - to + 1
        return m_Reversed ? ( m_From - m_To + 1 ) : ( m_To - m_From + 1 );
    }

    // map local view index along this axis back to tensor index
    size_t map ( size_t i ) const
    {
        // walk forward or backward along the stored range
        return m_Reversed ? ( m_From - i ) : ( m_From + i );
    }
};

template <typename T_>
class CTensorView;

template <typename T_, size_t DIM_>
class CTensor
{
  public:
    // fill tensor with val, sizes must match DIM_
    template <typename ... S>
    CTensor ( const T_ & val, S ... sizes )
    {
        // count of size args has to equal DIM_ or compile breaks
        static_assert ( sizeof...(sizes) == DIM_, "wrong number of sizes" );
        // turn the pack into plain size_ts
        size_t tmp [] = { static_cast<size_t> ( sizes ) ... };
        // multiply all sizes to know how big m_Data should be
        size_t total = 1;
        // copy sizes and total up the element count
        for ( size_t i = 0; i < DIM_; ++ i )
        {
            m_Sizes [ i ] = tmp [ i ];
            total *= tmp [ i ];
        }
        // make the buffer and fill it
        m_Data . assign ( total, val );
    }

    // 1D from initializer_list
    CTensor ( std::initializer_list<T_> il ) requires ( DIM_ == 1 )
    {
        // only one axis, length is how many values we got
        m_Sizes [ 0 ] = il . size ();
        // copy them in order
        m_Data . assign ( il . begin (), il . end () );
    }

    // 2D from nested initializer_list
    CTensor ( std::initializer_list<std::initializer_list<T_>> il ) requires ( DIM_ == 2 )
    {
        // height then width from the first row
        m_Sizes [ 0 ] = il . size ();
        m_Sizes [ 1 ] = il . size () ? il . begin () -> size () : 0;
        // every row same width or we throw
        for ( const auto & row : il )
        {
            if ( row . size () != m_Sizes [ 1 ] )
                throw std::invalid_argument ( "tensor is not a hyper-prism" );
            for ( const auto & x : row )
                m_Data . push_back ( x );
        }
    }

    // 3D from triple-nested initializer_list
    CTensor ( std::initializer_list<std::initializer_list<std::initializer_list<T_>>> il )
        requires ( DIM_ == 3 )
    {
        // depth along first axis
        m_Sizes [ 0 ] = il . size ();
        // guard empty tensor
        if ( m_Sizes [ 0 ] == 0 )
        {
            m_Sizes [ 1 ] = 0;
            m_Sizes [ 2 ] = 0;
            return;
        }
        // second axis from first block
        m_Sizes [ 1 ] = il . begin () -> size ();
        if ( m_Sizes [ 1 ] == 0 )
        {
            m_Sizes [ 2 ] = 0;
            return;
        }
        // third axis from first row of first block
        m_Sizes [ 2 ] = il . begin () -> begin () -> size ();
        // walk everything and check prism shape
        for ( const auto & block : il )
        {
            if ( block . size () != m_Sizes [ 1 ] )
                throw std::invalid_argument ( "tensor is not a hyper-prism" );
            for ( const auto & row : block )
            {
                if ( row . size () != m_Sizes [ 2 ] )
                    throw std::invalid_argument ( "tensor is not a hyper-prism" );
                for ( const auto & x : row )
                    m_Data . push_back ( x );
            }
        }
    }

    // row-major linear index from a full index vector
    size_t linear ( const std::array<size_t, DIM_> & ix ) const
    {
        // start at front of buffer
        size_t off = 0;
        // fold in each coordinate
        for ( size_t d = 0; d < DIM_; ++ d )
        {
            // product of all sizes after this axis
            size_t stride = 1;
            for ( size_t k = d + 1; k < DIM_; ++ k )
                stride *= m_Sizes [ k ];
            // add this axis contribution
            off += ix [ d ] * stride;
        }
        return off;
    }

    // 1D access (non-const)
    T_ & operator () ( size_t i ) requires ( DIM_ == 1 )
    {
        if ( i >= m_Sizes [ 0 ] )
            throw std::out_of_range ( "tensor index out of range" );
        return m_Data [ i ];
    }

    // 1D access (const)
    const T_ & operator () ( size_t i ) const requires ( DIM_ == 1 )
    {
        if ( i >= m_Sizes [ 0 ] )
            throw std::out_of_range ( "tensor index out of range" );
        return m_Data [ i ];
    }

    // 2D access (non-const)
    T_ & operator () ( size_t i, size_t j ) requires ( DIM_ == 2 )
    {
        if ( i >= m_Sizes [ 0 ] || j >= m_Sizes [ 1 ] )
            throw std::out_of_range ( "tensor index out of range" );
        std::array<size_t, 2> ix { i, j };
        return m_Data [ linear ( ix ) ];
    }

    // 2D access (const)
    const T_ & operator () ( size_t i, size_t j ) const requires ( DIM_ == 2 )
    {
        if ( i >= m_Sizes [ 0 ] || j >= m_Sizes [ 1 ] )
            throw std::out_of_range ( "tensor index out of range" );
        std::array<size_t, 2> ix { i, j };
        return m_Data [ linear ( ix ) ];
    }

    // 3D access (non-const)
    T_ & operator () ( size_t i, size_t j, size_t k ) requires ( DIM_ == 3 )
    {
        if ( i >= m_Sizes [ 0 ] || j >= m_Sizes [ 1 ] || k >= m_Sizes [ 2 ] )
            throw std::out_of_range ( "tensor index out of range" );
        std::array<size_t, 3> ix { i, j, k };
        return m_Data [ linear ( ix ) ];
    }

    // 3D access (const)
    const T_ & operator () ( size_t i, size_t j, size_t k ) const requires ( DIM_ == 3 )
    {
        if ( i >= m_Sizes [ 0 ] || j >= m_Sizes [ 1 ] || k >= m_Sizes [ 2 ] )
            throw std::out_of_range ( "tensor index out of range" );
        std::array<size_t, 3> ix { i, j, k };
        return m_Data [ linear ( ix ) ];
    }

    // build a view over this tensor
    template <typename ... A>
    CTensorView<T_> slice ( A ... args )
    {
        // cannot pass more slice args than we have dimensions
        static_assert ( sizeof...(args) <= DIM_, "too many slice arguments" );
        // one slice part per original axis
        std::vector<CSlicePart> parts ( DIM_ );
        // unpack the leading args into parts
        if constexpr ( sizeof...(args) > 0 )
            fillParts<0> ( parts, args ... );
        // anything we did not specify uses the full axis of the tensor
        for ( size_t i = sizeof...(args); i < DIM_; ++ i )
        {
            parts [ i ] . m_Fixed    = false;
            parts [ i ] . m_From     = 0;
            parts [ i ] . m_To       = m_Sizes [ i ] ? m_Sizes [ i ] - 1 : 0;
            parts [ i ] . m_Reversed = false;
        }
        // sanity check every part against real bounds
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
        // keep a raw pointer for the accessor lambda
        auto self = this;
        // lambda that turns a full original index vector into a real element ref
        auto acc = [ self ] ( const std::vector<size_t> & idx ) -> T_ &
        {
            if constexpr ( DIM_ == 1 )
                return ( * self ) ( idx [ 0 ] );
            else if constexpr ( DIM_ == 2 )
                return ( * self ) ( idx [ 0 ], idx [ 1 ] );
            else
                return ( * self ) ( idx [ 0 ], idx [ 1 ], idx [ 2 ] );
        };
        // view stores that lambda plus the slice description
        return CTensorView<T_> ( std::move ( acc ), DIM_, std::move ( parts ) );
    }

    // same brace layout as in the progtest strings
    friend std::ostream & operator << ( std::ostream & os, const CTensor & t )
    {
        if constexpr ( DIM_ == 1 )
        {
            // flat single line
            os << "{";
            for ( size_t i = 0; i < t . m_Sizes [ 0 ]; ++ i )
            {
                if ( i ) os << ", ";
                os << t . m_Data [ i ];
            }
            os << "}";
        }
        else if constexpr ( DIM_ == 2 )
        {
            // matrix style with two-space indent on rows
            os << "{\n";
            for ( size_t i = 0; i < t . m_Sizes [ 0 ]; ++ i )
            {
                os << "  {";
                for ( size_t j = 0; j < t . m_Sizes [ 1 ]; ++ j )
                {
                    if ( j ) os << ", ";
                    std::array<size_t, 2> ix { i, j };
                    os << t . m_Data [ t . linear ( ix ) ];
                }
                os << "}";
                if ( i + 1 < t . m_Sizes [ 0 ] ) os << ",";
                os << "\n";
            }
            os << "}";
        }
        else
        {
            // DIM_ == 3: extra nesting and four spaces on the inner rows
            os << "{\n";
            for ( size_t i = 0; i < t . m_Sizes [ 0 ]; ++ i )
            {
                os << "  {\n";
                for ( size_t j = 0; j < t . m_Sizes [ 1 ]; ++ j )
                {
                    os << "    {";
                    for ( size_t k = 0; k < t . m_Sizes [ 2 ]; ++ k )
                    {
                        if ( k ) os << ", ";
                        std::array<size_t, 3> ix { i, j, k };
                        os << t . m_Data [ t . linear ( ix ) ];
                    }
                    os << "}";
                    if ( j + 1 < t . m_Sizes [ 1 ] ) os << ",";
                    os << "\n";
                }
                os << "  }";
                if ( i + 1 < t . m_Sizes [ 0 ] ) os << ",";
                os << "\n";
            }
            os << "}";
        }
        return os;
    }

  private:
    // unpack slice() arguments into parts[0], parts[1], ...
    template <size_t I, typename F, typename ... R>
    void fillParts ( std::vector<CSlicePart> & parts, F f, R ... rest )
    {
        if constexpr ( std::is_integral_v<F> )
        {
            // plain number means fix this axis
            parts [ I ] . m_Fixed    = true;
            parts [ I ] . m_FixedVal = static_cast<size_t> ( f );
        }
        else if constexpr ( std::is_same_v<F, CRange> )
        {
            parts [ I ] . m_Fixed    = false;
            parts [ I ] . m_From     = f . m_From;
            parts [ I ] . m_To       = f . m_To;
            parts [ I ] . m_Reversed = false;
        }
        else if constexpr ( std::is_same_v<F, CRangeRev> )
        {
            parts [ I ] . m_Fixed    = false;
            parts [ I ] . m_From     = f . m_From;
            parts [ I ] . m_To       = f . m_To;
            parts [ I ] . m_Reversed = true;
        }
        if constexpr ( sizeof...(rest) > 0 )
            fillParts<I + 1> ( parts, rest ... );
    }

    // side lengths per axis
    std::array<size_t, DIM_>    m_Sizes;
    // actual storage
    std::vector<T_>             m_Data;
};


template <typename T_>
class CTensorView
{
  public:
    // acc reads/writes the backing tensor, origDim is its rank, parts describe the slice
    CTensorView ( std::function < T_ & ( const std::vector<size_t> & ) > acc,
                  size_t origDim,
                  std::vector<CSlicePart> parts )
        : m_Acc     ( std::move ( acc ) ),
          m_OrigDim ( origDim ),
          m_Parts   ( std::move ( parts ) )
    {
    }

    // variadic access in view coordinates (only the non-fixed axes)
    template <typename ... I>
    T_ & operator () ( I ... idx ) const
    {
        // pack what the caller passed
        std::array<size_t, sizeof...(idx)> user { static_cast<size_t> ( idx ) ... };
        // full index in the original tensor
        std::vector<size_t> mapped ( m_OrigDim );
        // how many user indices we already ate
        size_t u = 0;
        // walk original axes in order
        for ( size_t d = 0; d < m_OrigDim; ++ d )
        {
            if ( m_Parts [ d ] . m_Fixed )
            {
                // this axis does not take a view index
                mapped [ d ] = m_Parts [ d ] . m_FixedVal;
            }
            else
            {
                // need another index from the user
                if ( u >= user . size () )
                    throw std::out_of_range ( "not enough indices" );
                size_t v = user [ u ++ ];
                if ( v >= m_Parts [ d ] . length () )
                    throw std::out_of_range ( "view index out of range" );
                mapped [ d ] = m_Parts [ d ] . map ( v );
            }
        }
        // leftover user indices are an error
        if ( u != user . size () )
            throw std::out_of_range ( "too many indices" );
        return m_Acc ( mapped );
    }

    // format a view like a tensor of the same visible rank
    friend std::ostream & operator << ( std::ostream & os, const CTensorView & v )
    {
        // lengths of axes that are not fixed
        std::vector<size_t> freeSz;
        for ( const auto & p : v . m_Parts )
            if ( ! p . m_Fixed )
                freeSz . push_back ( p . length () );
        // 1D view: one line in braces
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
        // 2D view: same as 2D tensor printing
        else if ( freeSz . size () == 2 )
        {
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
        else
        {
            // 3 free axes -> same 3D layout as CTensor<...,3>
            os << "{\n";
            for ( size_t i = 0; i < freeSz [ 0 ]; ++ i )
            {
                os << "  {\n";
                for ( size_t j = 0; j < freeSz [ 1 ]; ++ j )
                {
                    os << "    {";
                    for ( size_t k = 0; k < freeSz [ 2 ]; ++ k )
                    {
                        if ( k ) os << ", ";
                        os << v ( i, j, k );
                    }
                    os << "}";
                    if ( j + 1 < freeSz [ 1 ] ) os << ",";
                    os << "\n";
                }
                os << "  }";
                if ( i + 1 < freeSz [ 0 ] ) os << ",";
                os << "\n";
            }
            os << "}";
        }
        return os;
    }

  private:
    // callback into the real tensor
    std::function < T_ & ( const std::vector<size_t> & ) > m_Acc;
    // rank of the tensor we sliced from
    size_t                      m_OrigDim;
    // per-axis slice info
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
  CTensor<int,3> m3
  {
    {
       { 10, 11, 12, 13, 14 },
       { 15, 16, 17, 18, 19 },
       { 20, 21, 22, 23, 24 },
       { 25, 26, 27, 28, 29 },
    },
    {
       { 31, 32, 33, 34, 35 },
       { 36, 37, 38, 39, 40 },
       { 41, 42, 43, 44, 45 },
       { 46, 47, 48, 49, 50 },
    },
    {
       { 60, 61, 62, 63, 64 },
       { 65, 66, 67, 68, 69 },
       { 70, 71, 72, 73, 74 },
       { 75, 76, 77, 78, 79 },
    }
  };
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

  assert ( toString ( m3 ) == R"({
  {
    {10, 11, 12, 13, 14},
    {15, 16, 17, 18, 19},
    {20, 21, 22, 23, 24},
    {25, 26, 27, 28, 29}
  },
  {
    {31, 32, 33, 34, 35},
    {36, 37, 38, 39, 40},
    {41, 42, 43, 44, 45},
    {46, 47, 48, 49, 50}
  },
  {
    {60, 61, 62, 63, 64},
    {65, 66, 67, 68, 69},
    {70, 71, 72, 73, 74},
    {75, 76, 77, 78, 79}
  }
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

  auto v3 = m3 . slice ( CRange ( 1, 2 ), CRange ( 0, 2 ), CRange ( 0, 3 ) );
  assert ( toString ( v3 ) == R"({
  {
    {31, 32, 33, 34},
    {36, 37, 38, 39},
    {41, 42, 43, 44}
  },
  {
    {60, 61, 62, 63},
    {65, 66, 67, 68},
    {70, 71, 72, 73}
  }
})" );

  auto v2 = m3 . slice ( 1, CRange ( 0, 2 ), CRange ( 0, 3 ) );
  assert ( toString ( v2 ) == R"({
  {31, 32, 33, 34},
  {36, 37, 38, 39},
  {41, 42, 43, 44}
})" );


  v2 = m3 . slice ( CRange ( 1, 2 ), 2, CRange ( 0, 3 ) );
  assert ( toString ( v2 ) == R"({
  {41, 42, 43, 44},
  {70, 71, 72, 73}
})" );

  v2 = m3 . slice ( CRange ( 1, 2 ), CRange ( 0, 2 ), 3 );
  assert ( toString ( v2 ) == R"({
  {34, 39, 44},
  {63, 68, 73}
})" );


  v2 = m3 . slice ( CRangeRev ( 2, 0 ), 1, CRange ( 4, 4 ) );
  assert ( toString ( v2 ) == R"({
  {69},
  {40},
  {19}
})" );

  v2 = m3 . slice ( CRangeRev ( 2, 0 ), 2 );
  assert ( toString ( v2 ) == R"({
  {70, 71, 72, 73, 74},
  {41, 42, 43, 44, 45},
  {20, 21, 22, 23, 24}
})" );

  assert ( v2 ( 1, 1 ) == 42 );
  v2 ( 1, 1 ) = 666;
  assert ( v2 ( 1, 1 ) == 666 );
  assert ( toString ( v2 ) == R"({
  {70, 71, 72, 73, 74},
  {41, 666, 43, 44, 45},
  {20, 21, 22, 23, 24}
})" );

  assert ( toString ( m3 ) == R"({
  {
    {10, 11, 12, 13, 14},
    {15, 16, 17, 18, 19},
    {20, 21, 22, 23, 24},
    {25, 26, 27, 28, 29}
  },
  {
    {31, 32, 33, 34, 35},
    {36, 37, 38, 39, 40},
    {41, 666, 43, 44, 45},
    {46, 47, 48, 49, 50}
  },
  {
    {60, 61, 62, 63, 64},
    {65, 66, 67, 68, 69},
    {70, 71, 72, 73, 74},
    {75, 76, 77, 78, 79}
  }
})" );

  auto v1 = m3 . slice ( 1, 2, CRange ( 1, 2 ) );
  assert ( toString ( v1 ) == "{666, 43}" );

  v1 = m3 . slice ( 1, 2, CRange ( 1, 1 ) );
  assert ( toString ( v1 ) == "{666}" );

  v1 = m3 . slice ( CRange ( 0, 2 ), 2, 3 );
  assert ( toString ( v1 ) == "{23, 44, 73}" );

  assert ( toString ( m4 . slice ( CRangeRev ( 1, 0 ), CRangeRev ( 1, 0 ) ) ) == R"({
  {PA2, PA1},
  {progtest, test}
})" );

  try
  {
    m3 ( 2, 3, 5 ) ++;
    assert ( "missing an exception" == nullptr );
  }
  catch ( const std::exception & e )
  {
  }

  try
  {
    v2 ( 2, 5 ) --;
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
