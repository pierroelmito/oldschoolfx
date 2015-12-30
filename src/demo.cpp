
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
	const sf::Uint8 p0 = demo::sample(p.data, x, y);
	const sf::Uint8 p1 = demo::sample(p.data, x + fakesin<int>(y + (20 * p.frame) / 16, 256), y);
	const sf::Uint8 p2 = demo::sample(p.data, x, y + fakesin<int>(x + (27 * p.frame) / 16, 256));
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

inline sf::Uint8 getAt(sf::Uint8* const rnd, int w, int h, int x, int y)
{
	const int nx = x % w;
	const int ny = y % h;
	return rnd[ny * w + nx];
}

inline sf::Uint8 sampleNoise(int x, int y, sf::Uint8* const rnd, int rw, int rh)
{
	float c = 0;
	float tw = 0;

	for (int i = 0; i <= 6; ++i)
	{
		const int nx = (x >> i) + 3 * i;
		const int ny = (y >> i) + 3 * i;
		const float tl = getAt(rnd, rw, rh, nx + 0, ny + 0) / 255.0f;
		const float tr = getAt(rnd, rw, rh, nx + 1, ny + 0) / 255.0f;
		const float bl = getAt(rnd, rw, rh, nx + 0, ny + 1) / 255.0f;
		const float br = getAt(rnd, rw, rh, nx + 1, ny + 1) / 255.0f;
		const int l = x & ~((1 << i) - 1);
		const int r = l + (1 << i);
		const int t = y & ~((1 << i) - 1);
		const int b = t + (1 << i);
		const float rx1 = slerpf(float(x - l) / float(r - l));
		const float ry1 = slerpf(float(y - t) / float(b - t));
		const float rx0 = 1.0f - rx1;
		const float ry0 = 1.0f - ry1;
		const float div = rx0 * ry0 + rx1 * ry0 + rx0 * ry1 + rx1 * ry1;
		const float fv = rx0 * ry0 * tl + rx1 * ry0 * tr + rx0 * ry1 * bl + rx1 * ry1 * br;

		const float w = powf(0.4f, 7 - i);
		tw += w;
		c += w * fv / div;
	}

	c /= tw;
	return int(255.99f * c);
}

// ---------------------------------------------------------------------------------------
// bump
// ---------------------------------------------------------------------------------------

// ---------------------------------------------------------------------------------------
// tunnel
// ---------------------------------------------------------------------------------------

// ---------------------------------------------------------------------------------------
// water ripples
// ---------------------------------------------------------------------------------------

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
	auto fb8 = win.createBuffer<sf::Uint8>();
	auto fb16 = win.createBuffer<sf::Uint16>();

	// fx buffers
	auto cc = new demo::buffer<sf::Uint8, ScrWidth, ScrHeight, demo::procst<sf::Uint8, ScrWidth, ScrHeight, ccparams, computeCC>>();
	auto rotozoom = new demo::buffer<sf::Uint8, ScrWidth, ScrHeight, demo::procst<sf::Uint8, ScrWidth, ScrHeight, rzparams, computeRotozoom>>();
	auto plasma = new demo::buffer<sf::Uint8, ScrWidth, ScrHeight, demo::procst<sf::Uint8, ScrWidth, ScrHeight, plasmaparams, computePlasma>>();

	// images
	auto pipo = demo::makeBuffer<sf::Uint8, 256, 256>(samplePlasma);
	auto mito = demo::makeBuffer<sf::Uint8, 256, 256>(sampleRZ);
	sf::Uint8 rndNoise[16 * 10] = { 0 };
	for (int y = 0, offset = 0; y < 10; ++y)
		for (int x = 0; x < 16; ++x)
			rndNoise[offset++] = ((x ^ y) & 1) * 64 + rand() % 192;
	//std::generate(rndNoise, rndNoise + sizeof(rndNoise), [] () { return rand() % 256; });
	auto bidon = demo::makeBuffer<sf::Uint8, 320, 200>(sampleNoise, &rndNoise[0], 16, 10);

	// palettes
	auto palNoise = demo::makeRampPal<sf::Uint32, 256>( { 0xff000000, 0xffffffff } );
	auto palCC = demo::makeRampPal<sf::Uint32, 256>( { 0xff00ff00, 0xffff00ff } );
	auto palRZ = demo::makeRampPal<sf::Uint32, 256>( { 0xff0000ff, 0xffffff00 } );
	auto palPlasma = demo::makeRampPal<sf::Uint32, 256>( { 0xffff0000, 0xff0000ff, 0xff00ffff, 0xffff0000 } );
	auto palFire = demo::makeRampPal<sf::Uint32, 256>( { 0xff000000, 0xff0000ff, 0xff00ffff, 0xffffffff, 0xffffffff } );

	typedef std::function<void(tWin320x200::tBackBuffer&, int)> tFxFunc;

	// bump 
	tFxFunc bumpFunc = [&] (tWin320x200::tBackBuffer& bgFb, int frame) {
		bgFb.transformOfs(fb8->copyXY(*bidon), [&palNoise] (sf::Uint8 l) { return palNoise[l]; });
	};

	// plasma
	tFxFunc plasmaFunc = [&] (tWin320x200::tBackBuffer& bgFb, int frame) {
		plasma->_params = {
			pipo->data(),
			frame,
		};
		bgFb.transformOfs(fb8->copyXY(*plasma), [&palPlasma] (sf::Uint8 l) { return palPlasma[l]; });
	};

	// rotozoom
	tFxFunc rzFunc = [&] (tWin320x200::tBackBuffer& bgFb, int frame) {
		const float rzf = 0.5f * frame;           // time
		const float a = 0.03f * rzf;              // angle
		const float z = 1.2f + cosf(0.05f * rzf); // zoom
		const float r = 500.0f;                   // move radius
		rotozoom->_params = {
			mito->data(),
			int(128.0f + 256.0f * r * cosf(0.03f * rzf)) , int(128.0f + 256.0f * r * cosf(0.04f * rzf)), // center
			int(256.0f * z * cosf(a)), int(256.0f * z * sinf(a)),                                        // direction
		};
		bgFb.transformOfs(fb8->copyXY(*rotozoom), [&palRZ] (sf::Uint8 l) { return palRZ[l]; });
	};

	// fire
	tFxFunc fireFunc = [&] (tWin320x200::tBackBuffer& bgFb, int frame) {
		setFire(fb16->data(), frame);
		bgFb.transformOfs(*fb16, [&palFire] (sf::Uint16 l) { return palFire[l >> 8]; });
	};

	// circles
	tFxFunc ccFunc = [&] (tWin320x200::tBackBuffer& bgFb, int frame) {
		cc->_params = {
			mito->data(),
			int(160 + 150 * sinf(0.03f * frame)), 100, // first pos
			160, int(100 + 90 * sinf(0.04f * frame)),  // second pos
		};
		bgFb.transformOfs(fb8->copyXY(*cc), [&palCC] (sf::Uint8 l) { return palCC[l]; });
	};

	// FX list
	tFxFunc fxs[] = {
		bumpFunc,
		rzFunc,
		ccFunc,
		plasmaFunc,
		fireFunc,
	};

	// run loop
	win.run([&] (tWin320x200::tBackBuffer& bgFb, int frame) {
		const int fxCount = sizeof(fxs) / sizeof(fxs[0]);
		const int fxDuration = 200;
		if (frame >= fxDuration * fxCount)
			return false;
		int fxIdx = frame / fxDuration;
		fxs[fxIdx](bgFb, frame);
		return true;
	});
	
	return 0;
}

