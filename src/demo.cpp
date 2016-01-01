
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>

#include <functional>

#define SYNC_60Hz 1

namespace demo
{

template <typename T, int W, int H>
class frameb
{
public:
	template <typename... P>
	void init(T f(int, int, P...), P... p)
	{
		for (int y = 0, offset = 0; y < H; ++y)
			for (int x = 0; x < W; ++x)
				_data[offset++] = f(x, y, p...);
	}
	T& xy(int x, int y) { return _data[y * W + x]; }
	T& ofs(int o) { return _data[o]; }
	const T& xy(int x, int y) const { return _data[y * W + x]; }
	const T& ofs(int o) const { return _data[o]; }
	T* data() { return _data; }
private:
	T _data[W * (H + 1)];
};

template <typename T, int W, int H, T (*F)(int, int)>
class procfn
{
public:
	T xy(int x, int y) const { return F(x, y); }
	T ofs(int o) const { return F(o % W, o / W); }
};

template <typename T, int W, int H, class P, T (*F)(int, int, const P&)>
class procst
{
public:
	P _params;
	T xy(int x, int y) const { return F(x, y, _params); }
	T ofs(int o) const { return F(o % W, o / W, _params); }
};

template <typename T, int W, int H, class I>
class buffer : public I
{
public:
	typedef buffer<T, W, H, I> tBuffer;

	using I::xy;
	using I::ofs;

	template <class U>
	tBuffer& copyXY(const U& src)
	{
		for (int y = 0, o = 0; y < H; ++y)
			for (int x = 0; x < W; ++x, ++o)
				ofs(o) = src.xy(x, y);
		return *this;
	}

	template <class U>
	tBuffer& copyOfs(const U& src)
	{
		for (int o = 0; o < W * H; ++o)
			ofs(o) = src.ofs(o);
		return *this;
	}

	template <typename T0, class I0, typename F>
	tBuffer& transformXY(const buffer<T0, W, H, I0>& src0, const F& func)
	{
		for (int y = 0, o = 0; y < H; ++y)
			for (int x = 0; x < W; ++x, ++o)
				ofs(o) = func(src0.xy(x, y));
		return *this;
	}

	template <typename T0, class I0, typename F>
	tBuffer& transformOfs(const buffer<T0, W, H, I0>& src0, const F& func)
	{
		for (int o = 0; o < W * H; ++o)
			ofs(o) = func(src0.ofs(o));
		return *this;
	}

	template <typename T0, class I0, typename T1, class I1, typename F>
	tBuffer& transformXY(const buffer<T0, W, H, I0>& src0, const buffer<T1, W, H, I1>& src1, const F& func)
	{
		for (int y = 0, o = 0; y < H; ++y)
			for (int x = 0; x < W; ++x, ++o)
				ofs(o) = func(src0.xy(x, y), src1.xy(x, y));
		return *this;
	}

	template <typename T0, class I0, typename T1, class I1, typename F>
	tBuffer& transformOfs(const buffer<T0, W, H, I0>& src0, const buffer<T1, W, H, I1>& src1, const F& func)
	{
		for (int o = 0; o < W * H; ++o)
			ofs(o) = func(src0.ofs(o), src1.ofs(o));
		return *this;
	}
};

template <int W, int H>
class demowin
{
public:
	typedef buffer<sf::Uint32, W, H, demo::frameb<sf::Uint32, W, H>> tBackBuffer;

	demowin()
	{
	}

	template <class T>
	buffer<T, W, H, demo::frameb<T, W, H>>* createBuffer()
	{
		return new buffer<T, W, H, demo::frameb<T, W, H>>();
	}

	template <class F>
	void run(const F& f)
	{
		auto bgFb = new buffer<sf::Uint32, W, H, demo::frameb<sf::Uint32, W, H>>();

		sf::RenderWindow win(sf::VideoMode(W, H), "toto");
#if SYNC_60Hz
		win.setFramerateLimit(60);
		win.setVerticalSyncEnabled(true);
#endif

		sf::Texture bg;
		bg.create(W, H);
		sf::Sprite bgSp(bg);

		sf::View view = win.getDefaultView();
		sf::Vector2u winSize;

		int frame = 0;
		while (win.isOpen())
		{
			sf::Event e;
			while (win.pollEvent(e))
			{
				switch (e.type)
				{
				case sf::Event::Closed:
					win.close();
					break;
				case sf::Event::Resized:
					winSize = sf::Vector2u(e.size.width, e.size.height);
					view.reset(sf::FloatRect(0.0f, 0.0f, e.size.width, e.size.height));
					win.setView(view);
					win.setSize(winSize);
					break;
				default:
					break;
				}
			}

			if (!f(*bgFb, frame))
				win.close();
			bg.update((sf::Uint8*)&bgFb->ofs(0));

			float s0 = winSize.x / float(W);
			float s1 = winSize.y / float(H);
			float s = std::min(s0, s1);
			float x = 0.5f * (winSize.x - s * W);
			float y = 0.5f * (winSize.y - s * H);
			bgSp.setScale(s, s);
			bgSp.setPosition(x, y);

			win.clear(sf::Color::Black);
			win.draw(bgSp);
			win.display();

			++frame;
		}
	}
};

inline sf::Uint32 abgr(sf::Uint8 a, sf::Uint8 b, sf::Uint8 g, sf::Uint8 r)
{
	return (a << 24) | (b << 16) | (g << 8) | r;
}

inline sf::Uint32 modulate(sf::Uint32 a, sf::Uint32 b)
{
	typedef sf::Uint8 tColor[4];
	sf::Uint32 r;
	const tColor& c = *reinterpret_cast<const tColor*>(&a);
	const tColor& d = *reinterpret_cast<const tColor*>(&b);
	tColor& e = *reinterpret_cast<tColor*>(&r);
	for (int i = 0; i < 4; ++i)
		e[i] = sf::Uint8(c[i] * d[i] / 255u);
	return r;
}

inline sf::Uint32 blend(sf::Uint32 a, sf::Uint32 b, int mul, int div)
{
	typedef sf::Uint8 tColor[4];
	sf::Uint32 r;
	const tColor& c = *reinterpret_cast<const tColor*>(&a);
	const tColor& d = *reinterpret_cast<const tColor*>(&b);
	tColor& e = *reinterpret_cast<tColor*>(&r);
	for (int i = 0; i < 4; ++i)
		e[i] = sf::Uint8(((div - mul) * c[i] + mul * d[i]) / div);
	return r;
}

inline bool palRamp(int s, int e, int i, sf::Uint32 cs, sf::Uint32 ce, sf::Uint32* r)
{
	if (i >= s && i <= e)
	{
		*r = blend(cs, ce, i - s, e - s);
		return true;
	}
	return false;
}

template <class T, size_t N, class F>
std::array<T, N> makePal(const F& f)
{
	std::array<T, N> r;
	for (size_t i = 0; i < N; ++i)
		r[i] = f(i);
	return r;
}

template <class T, size_t N>
std::array<T, N> makeRampPal(const std::initializer_list<T>& l)
{
	std::array<T, N> r;
	auto i0 = l.begin();
	auto i1 = i0 + 1;
	for (size_t i = 0; i < l.size() - 1; ++i)
	{
		T cs = *i0++;
		T ce = *i1++;
		size_t s = (256 * (i + 0)) / (l.size() - 1);
		size_t e = (256 * (i + 1)) / (l.size() - 1);
		for (size_t j = s; j < e; ++j)
			r[j] = blend(cs, ce, j - s, e - 1 - s);
	}
	return r;
}

template <typename T, int W, int H, typename... P>
buffer<T, W, H, frameb<T, W, H>>* makeBuffer(T f(int, int, P...), P... p)
{
	auto r = new demo::buffer<T, W, H, demo::frameb<T, W, H>>();
	int offset = 0;
	for (int y = 0; y < H; ++y)
		for (int x = 0; x < W; ++x)
			r->data()[offset++] = f(x, y, p...);
	// duplicate last horizontal line
	for (int x = 0; x < W; ++x)
		r->data()[W * H + x] = r->data()[W * (H - 1) + x];
	return r;
}

sf::Uint8 r8(int v, int a, int b)
{
	return sf::Uint8((255u * (v - a)) / (b - a));
}

inline sf::Uint8 sample(sf::Uint8* d, int x, int y)
{
	return d[((x & 255) << 8) | (y & 255)];
}

}

// window size
const int ScrWidth = 320;
const int ScrHeight = 200;

template <typename T>
inline T slerpi(T x, T b)
{
	return (3 * x * x * b - 2 * x * x * x) / (b * b);
}

template <typename T>
inline T slerpf(T x)
{
	return 3 * x * x - 2 * x * x * x;
}

template <typename T>
inline T fakesin(T x, T b)
{
	const T nx = x & (b - 1);
	const T nb = b / 2;
	return (nx < nb) ?  slerpi<T>(nx, nb) : nb - slerpi<T>(nx - nb, nb);
}

// ---------------------------------------------------------------------------------------
// circles
// ---------------------------------------------------------------------------------------

struct ccparams
{
	sf::Uint8* data;
	int x0, y0, x1, y1;
};

inline int dist(int x0, int y0, int x1, int y1)
{
	int dx = x1 - x0;
	int dy = y1 - y0;
	return dx * dx + dy * dy;
}

static inline sf::Uint8 computeCC(int x, int y, const ccparams& p)
{
	int u = dist(p.x0, p.y0, x, y) / 512;
	int v = dist(p.x1, p.y1, x, y) / 512;
	return demo::sample(p.data, u, v);
}

// ---------------------------------------------------------------------------------------
// rotozoom
// ---------------------------------------------------------------------------------------

struct rzparams
{
	sf::Uint8* data;
	int cx, cy; // center
	int dx, dy; // direction
};

inline sf::Uint8 sampleRZ(int x, int y)
{
	return x ^ y;
}

static inline sf::Uint8 computeRotozoom(int x, int y, const rzparams& p)
{
	x -= ScrWidth / 2;
	y -= ScrHeight / 2;
	int nx = (p.cx + x * p.dx - y * p.dy) / 256;
	int ny = (p.cy + x * p.dy + y * p.dx) / 256;
	return demo::sample(p.data, nx, ny);
}

// ---------------------------------------------------------------------------------------
// plasma
// ---------------------------------------------------------------------------------------

inline sf::Uint8 samplePlasma(int x, int y)
{
	return fakesin<int>(x, 256) + fakesin<int>(y, 256);
}

struct plasmaparams
{
	sf::Uint8* data;
	int frame;
};

static inline sf::Uint8 computePlasma(int x, int y, const plasmaparams& p)
{
	const sf::Uint8 p0 = demo::sample(p.data, x + y / 2, 3 * y / 2);
	const sf::Uint8 p1 = demo::sample(p.data, x + fakesin<int>(y + (20 * p.frame) / 16, 256), y);
	const sf::Uint8 p2 = demo::sample(p.data, 5 * x / 4, y + fakesin<int>(x + (27 * p.frame) / 16, 256));
	return p0 + p1 + p2;
}

// ---------------------------------------------------------------------------------------
// fire
// ---------------------------------------------------------------------------------------
inline void setFire(sf::Uint16* d, int frame)
{
	// use 16bpp buffer to increase quality

	// update random values at bottom pixel line
	if (frame % 4 == 0)
	{
		for (int j = 0; j < ScrWidth;)
		{
			sf::Uint8 r = 192 + 63 * (rand() % 2);
			// 10 pixels bloc
			for (int i = 0; i < 10; ++i, ++j)
				d[(ScrHeight - 1) * ScrWidth + j] = (r << 8) + rand() % 256;
		}
	}

	// two loops per frame for quicker fire
	for (int j = 0; j < 2; ++j)
	{
		for (int i = 0; i < (ScrHeight - 1) * ScrWidth; ++i)
		{
			// blur upward in place
			d[i] = (2 * d[i] + 1 * d[i + ScrWidth - 1] + 3 * d[i + ScrWidth] + 2 * d[i + ScrWidth + 1]) / 8;
			if (d[i] > 255)
				d[i] -= 256;
		}
	}
}

// ---------------------------------------------------------------------------------------
// noise
// ---------------------------------------------------------------------------------------

inline sf::Uint8 getAtWrap(const sf::Uint8* const rnd, int w, int h, int x, int y)
{
	const int nx = x % w;
	const int ny = y % h;
	//const int nx = x >= 0 ? (x % w) : -(-x % w);
	//const int ny = y >= 0 ? (y % h) : -(-y % h);
	return rnd[ny * w + nx];
}

inline sf::Uint8 getAtClamp(const sf::Uint8* const rnd, int w, int h, int x, int y)
{
	const int nx = std::max(0, std::min(w - 1, x));
	const int ny = std::max(0, std::min(h - 1, y));
	return rnd[ny * w + nx];
}

sf::Uint8 computeNoise(int x, int y, int w, int h)
{
	return (rand() % 1000 > 750 ? 224 : 0) + rand() % 32;
}

void fillNoise(sf::Uint8* rndNoise, int w, int h)
{
	for (int y = 0, offset = 0; y < h; ++y)
		for (int x = 0; x < w; ++x)
			rndNoise[offset++] = computeNoise(x, y, w, h);
}

inline sf::Uint8 sampleNoise(int x, int y, sf::Uint8* const rnd, int rw, int rh, int steps)
{
	float c = 0;
	float tw = 0;

	for (int i = 0; i <= steps; ++i)
	{
		const int nx = (x >> i) + i * 2;
		const int ny = (y >> i) + i * 2;

		const float tl = getAtWrap(rnd, rw, rh, nx + 0, ny + 0) / 255.0f; // top left value
		const float tr = getAtWrap(rnd, rw, rh, nx + 1, ny + 0) / 255.0f; // top right value
		const float bl = getAtWrap(rnd, rw, rh, nx + 0, ny + 1) / 255.0f; // bottom left value
		const float br = getAtWrap(rnd, rw, rh, nx + 1, ny + 1) / 255.0f; // bottom right value

		const int l = x & ~((1 << i) - 1);
		const int t = y & ~((1 << i) - 1);
		const int r = l + (1 << i);
		const int b = t + (1 << i);

		// compute bilinear coefficients
		const float rx1 = slerpf(float(x - l) / float(r - l));
		const float ry1 = slerpf(float(y - t) / float(b - t));
		const float rx0 = 1.0f - rx1;
		const float ry0 = 1.0f - ry1;

		const float fv = rx0 * ry0 * tl + rx1 * ry0 * tr + rx0 * ry1 * bl + rx1 * ry1 * br;

		const float w = powf(0.4f, steps + 1 - i);
		tw += w;
		c += w * fv;
	}

	c /= tw;
	return int(255.99f * c);
}

// ----------50--------------------------------------------------------------------------
// bump
// ---------------------------------------------------------------------------------------

void bump(sf::Uint8* dst, const sf::Uint8* src, int W, int H, int lposx, int lposy)
{
	for (int y = 0, offset = 0; y < H; ++y)
	{
		for (int x = 0; x < W; ++x, ++offset)
		{
			int nx = int(src[offset + 1]) - int(src[offset]);
			int ny = int(src[offset + W]) - int(src[offset]);
			int lx = lposx - x;
			int ly = lposy - y;
			int sql = 256 + (lx * lx + ly * ly);
			int l = 2048 * (nx * lx + ny * ly) / sql;
			l = l >= 0 ? (l > 255 ? 255 : l) : 0;
			dst[offset] = l;
		}
	}
}

// ---------------------------------------------------------------------------------------
// tunnel
// ---------------------------------------------------------------------------------------

// ---------------------------------------------------------------------------------------
// water ripples
// ---------------------------------------------------------------------------------------

typedef sf::Uint16 tWaterHeight;

constexpr tWaterHeight MediumHeight = (1 << ((8 * sizeof(tWaterHeight)) - 1)) - 1;

void waterPlot(tWaterHeight* dst, int W, int H, int cx, int cy, int r)
{
	for (int y = cy - r; y < cy + r; ++y)
	{
		for (int x = cx - r; x < cx + r; ++x)
		{
			int d2 = dist(x, y, cx, cy);
			int r2 = r * r;
			if (d2 < r2)
				dst[y * W + x] = MediumHeight + (MediumHeight * d2) / r2;
		}
	}
}

void waterInit(tWaterHeight* dst, int W, int H)
{
	for (int offset = 0; offset < W * (H + 1); ++offset)
		dst[offset] = MediumHeight ;
}

void waterMove(tWaterHeight* dst, const tWaterHeight* src, int W, int H)
{
	for (int y = 0, offset = 0; y < H; ++y)
	{
		for (int x = 0; x < W; ++x, ++offset)
		{
			const int dx0 = x == 0     ? 0 : -1;
			const int dx1 = x == W - 1 ? 0 :  1;
			const int dy0 = y == 0     ? 0 : -W;
			const int dy1 = y == H - 1 ? 0 :  W;

			const int vt = int(src[offset + dy0]);
			const int vb = int(src[offset + dy1]);
			const int vl = int(src[offset + dx0]);
			const int vr = int(src[offset + dx1]);

			const int vtl = int(src[offset + dy0 + dx0]);
			const int vtr = int(src[offset + dy0 + dx1]);
			const int vbl = int(src[offset + dy1 + dx0]);
			const int vbr = int(src[offset + dy1 + dx1]);

			const int s = (3 * (vt + vb + vl + vr) + 2 * (vtl + vtr + vbl + vbr)) / (3 * 4 + 2 * 4);

			const int nv = 2 * s - dst[offset];
			const int div = 32;
			dst[offset] = (MediumHeight  + ((div - 1) * nv)) / div;
		}
	}
}

template <typename T>
void waterDistort(T* dst, const T* src, const tWaterHeight* hm, int W, int H, int mul, int div)
{
	for (int y = 0, offset = 0; y < H; ++y)
	{
		for (int x = 0; x < W; ++x, ++offset)
		{
			int nx = x + mul * (int(hm[offset + 1]) - int(hm[offset])) / div;
			int ny = y + mul * (int(hm[offset + W]) - int(hm[offset])) / div;
			dst[offset] = getAtClamp(src, W, H, nx, ny);
		}
	}
}

// ---------------------------------------------------------------------------------------
// bars
// ---------------------------------------------------------------------------------------

// ---------------------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------------------

int main(int, char**)
{
	// open window
	typedef demo::demowin<ScrWidth, ScrHeight> tWin320x200;
	tWin320x200 win;

	// back buffers
	auto fb16a = win.createBuffer<sf::Uint16>();
	auto fb16b = win.createBuffer<sf::Uint16>();
	auto fb8a = win.createBuffer<sf::Uint8>();
	//auto fb8b = win.createBuffer<sf::Uint8>();
	//auto fb8c = win.createBuffer<sf::Uint8>();

	// fx buffers
	auto cc       = new demo::buffer<sf::Uint8, ScrWidth, ScrHeight, demo::procst<sf::Uint8, ScrWidth, ScrHeight, ccparams, computeCC>>();
	auto rotozoom = new demo::buffer<sf::Uint8, ScrWidth, ScrHeight, demo::procst<sf::Uint8, ScrWidth, ScrHeight, rzparams, computeRotozoom>>();
	auto plasma   = new demo::buffer<sf::Uint8, ScrWidth, ScrHeight, demo::procst<sf::Uint8, ScrWidth, ScrHeight, plasmaparams, computePlasma>>();

	// images
	const int NW = 40, NH = 25;
	sf::Uint8 rndNoise[NW * NH] = { 0 };
	fillNoise(rndNoise, NW, NH);
	auto bidon = demo::makeBuffer<sf::Uint8, 320, 200>(sampleNoise, &rndNoise[0], NW, NH, 6);
	auto pipo  = demo::makeBuffer<sf::Uint8, 256, 256>(samplePlasma);
	auto mito  = demo::makeBuffer<sf::Uint8, 256, 256>(sampleRZ);

	// palettes
	auto palGrey   = demo::makeRampPal<sf::Uint32, 256>( { 0xff000000, 0xffffffff } );
	auto palBump   = demo::makeRampPal<sf::Uint32, 256>( { 0xff0f0000, 0xff00007f, 0xff7fffff, 0xffffffff } );
	auto palCC     = demo::makeRampPal<sf::Uint32, 256>( { 0xff00ff00, 0xffff00ff } );
	auto palRZ     = demo::makeRampPal<sf::Uint32, 256>( { 0xff0000ff, 0xffffff00 } );
	auto palPlasma = demo::makeRampPal<sf::Uint32, 256>( { 0xffff0000, 0xff0000ff, 0xff00ffff, 0xffff0000 } );
	auto palFire   = demo::makeRampPal<sf::Uint32, 256>( { 0xff000000, 0xff0000ff, 0xff00ffff, 0xffffffff, 0xffffffff } );

	typedef std::function<void(tWin320x200::tBackBuffer&, int)> tFxFunc;

	waterInit(fb16a->data(), ScrWidth, ScrHeight); 
	waterInit(fb16b->data(), ScrWidth, ScrHeight); 

	// water
	tFxFunc waterFunc = [&] (tWin320x200::tBackBuffer& bgFb, int frame) {
		float sc = 0.03f;
		bool b = (frame % 2 == 0);
		auto b0 = b ? fb16a : fb16b;
		auto b1 = b ? fb16b : fb16a;
		waterPlot(
			b1->data(),
			ScrWidth,
			ScrHeight,
			ScrWidth * (0.5f * (1.0f + 0.8f * cosf(sc * frame))),
			ScrHeight * (0.5f * (1.0f + 0.8f * sinf(1.2f * sc * frame))),
			10
		);
		waterMove(b0->data(), b1->data(), ScrWidth, ScrHeight);
		waterDistort(fb8a->data(), bidon->data(), b0->data(), ScrWidth, ScrHeight, 4, 16 * 256);
		bgFb.transformOfs(*fb8a, [&] (sf::Uint8 l) { return palGrey[l]; });
	};

	// bump 
	tFxFunc bumpFunc = [&] (tWin320x200::tBackBuffer& bgFb, int frame) {
		float sc = 0.03f;
		bump(
			fb8a->data(),
			bidon->data(),
			ScrWidth,
			ScrHeight,
			ScrWidth * (0.5f * (1.0f + 0.9f * cosf(sc * frame))),
			ScrHeight * (0.5f * (1.0f + 0.9f * sinf(1.2f * sc * frame)))
		);
		bgFb.transformOfs(*fb8a, [&] (sf::Uint8 l) { return palBump[l]; });
	};

	// plasma
	tFxFunc plasmaFunc = [&] (tWin320x200::tBackBuffer& bgFb, int frame) {
		plasma->_params = {
			pipo->data(),
			frame,
		};
		bgFb.transformOfs(fb8a->copyXY(*plasma), [&] (sf::Uint8 l) { return palPlasma[l]; });
	};

	// rotozoom
	tFxFunc rzFunc = [&] (tWin320x200::tBackBuffer& bgFb, int frame) {
		const float rzf = 0.5f * frame;           // time
		const float a = 0.03f * rzf;              // angle
		const float z = 1.2f + cosf(0.05f * rzf); // zoom
		const float r = 256.0f * 500.0f;                   // move radius
		rotozoom->_params = {
			mito->data(),
			int(128.0f + r * cosf(0.03f * rzf)) , int(128.0f + r * cosf(0.04f * rzf)), // center
			int(256.0f * z * cosf(a)), int(256.0f * z * sinf(a)),                      // direction
		};
		bgFb.transformOfs(fb8a->copyXY(*rotozoom), [&] (sf::Uint8 l) { return palRZ[l]; });
	};

	// fire
	tFxFunc fireFunc = [&] (tWin320x200::tBackBuffer& bgFb, int frame) {
		setFire(fb16a->data(), frame);
		bgFb.transformOfs(*fb16a, [&] (sf::Uint16 l) { return palFire[l >> 8]; });
	};

	// circles
	tFxFunc ccFunc = [&] (tWin320x200::tBackBuffer& bgFb, int frame) {
		cc->_params = {
			mito->data(),
			int(160 + 150 * sinf(0.03f * frame)), 100, // first pos
			160, int(100 + 90 * sinf(0.04f * frame)),  // second pos
		};
		bgFb.transformOfs(fb8a->copyXY(*cc), [&] (sf::Uint8 l) { return palCC[l]; });
	};

	// FX list
	tFxFunc fxs[] = {
		waterFunc,
		bumpFunc,
		plasmaFunc,
		rzFunc,
		ccFunc,
		fireFunc,
	};

	// run loop
	win.run([&] (tWin320x200::tBackBuffer& bgFb, int frame) {
		const int fxCount = sizeof(fxs) / sizeof(fxs[0]);
		const int fxDuration = 2000;
		if (frame >= fxDuration * fxCount)
			return false;
		int fxIdx = frame / fxDuration;
		fxs[fxIdx](bgFb, frame);
		return true;
	});
	
	return 0;
}

