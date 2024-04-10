// ============
// SJTU SCC
// ============



#ifndef PMVP
#define PMVP

#if IBC_ME_FROM_VTM
typedef int PosType;
typedef uint32_t SizeType;
struct Position
{
  PosType x;
  PosType y;

  Position() : x(0), y(0) { }
  Position(const PosType _x, const PosType _y) : x(_x), y(_y) { }

  bool operator!=(const Position &other)  const { return x != other.x || y != other.y; }
  bool operator==(const Position &other)  const { return x == other.x && y == other.y; }

  Position offset(const Position pos)                 const { return Position(x + pos.x, y + pos.y); }
  Position offset(const PosType _x, const PosType _y) const { return Position(x + _x, y + _y); }
  void     repositionTo(const Position newPos) { x = newPos.x; y = newPos.y; }
  void     relativeTo(const Position origin) { x -= origin.x; y -= origin.y; }

  Position operator-(const Position &other)         const { return{ x - other.x, y - other.y }; }
};

struct TSize
{
  SizeType width;
  SizeType height;

  TSize() : width(0), height(0) { }
  TSize(const SizeType _width, const SizeType _height) : width(_width), height(_height) { }

  bool operator!=(const TSize &other)      const { return (width != other.width) || (height != other.height); }
  bool operator==(const TSize &other)      const { return (width == other.width) && (height == other.height); }
  uint32_t area()                             const { return (uint32_t)width * (uint32_t)height; }
};

struct Area : public Position, public TSize
{
  Area() : Position(), TSize() { }
  Area(const Position &_pos, const TSize &_size) : Position(_pos), TSize(_size) { }
  Area(const PosType _x, const PosType _y, const SizeType _w, const SizeType _h) : Position(_x, _y), TSize(_w, _h) { }

  Position& pos() { return *this; }
  const Position& pos()                     const { return *this; }
  TSize&     size() { return *this; }
  const TSize&     size()                    const { return *this; }

  const Position& topLeft()                 const { return *this; }
  Position  topRight()                const { return { (PosType)(x + width - 1), y }; }
  Position  bottomLeft()              const { return { x                        , (PosType)(y + height - 1) }; }
  Position  bottomRight()             const { return { (PosType)(x + width - 1), (PosType)(y + height - 1) }; }
  Position  center()                  const { return { (PosType)(x + width / 2), (PosType)(y + height / 2) }; }

  bool contains(const Position &_pos)       const { return (_pos.x >= x) && (_pos.x < (x + width)) && (_pos.y >= y) && (_pos.y < (y + height)); }
  bool contains(const Area &_area)          const { return contains(_area.pos()) && contains(_area.bottomRight()); }

  bool operator!=(const Area &other)        const { return (TSize::operator!=(other)) || (Position::operator!=(other)); }
  bool operator==(const Area &other)        const { return (TSize::operator==(other)) && (Position::operator==(other)); }
};
#endif

#endif