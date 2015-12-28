
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>

namespace demo
{

template <typename T, int W, int H>
class frameb
{
public:
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
		win.setFramerateLimit(60);
		win.setVerticalSyncEnabled(true);

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
	tColor& c = *reinterpret_cast<tColor*>(&a);
	tColor& d = *reinterpret_cast<tColor*>(&b);
	sf::Uint32 r;
	tColor& e = *reinterpret_cast<tColor*>(&r);
	e[0] = sf::Uint8(c[0] * d[0] / 255u);
	e[1] = sf::Uint8(c[1] * d[1] / 255u);
	e[2] = sf::Uint8(c[2] * d[2] / 255u);
	e[3] = sf::Uint8(c[3] * d[3] / 255u);
	return r;
}

inline sf::Uint32 blend(sf::Uint32 a, sf::Uint32 b, int mul, int div)
{
	typedef sf::Uint8 tColor[4];
	tColor& c = *reinterpret_cast<tColor*>(&a);
	tColor& d = *reinterpret_cast<tColor*>(&b);
	sf::Uint32 r;
	tColor& e = *reinterpret_cast<tColor*>(&r);
	e[0] = sf::Uint8(((div - mul) * c[0] + mul * d[0]) / div);
	e[1] = sf::Uint8(((div - mul) * c[1] + mul * d[1]) / div);
	e[2] = sf::Uint8(((div - mul) * c[2] + mul * d[2]) / div);
	e[3] = sf::Uint8(((div - mul) * c[3] + mul * d[3]) / div);
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

sf::Uint8 r8(int v, int a, int b)
{
	return sf::Uint8((255u * (v - a)) / (b - a));
}

}

// window size
const int ScrWidth = 320;
const int ScrHeight = 200;

// ---------------------------------------------------------------------------------------
// circles
// ---------------------------------------------------------------------------------------

struct ccparams
{
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
	return u ^ v;
}

// ---------------------------------------------------------------------------------------
// rotozoom
// ---------------------------------------------------------------------------------------

struct rzparams
{
	int cx, cy;
	int dx, dy;
};

static inline sf::Uint8 computeRotozoom(int x, int y, const rzparams& p)
{
	x -= ScrWidth / 2;
	y -= ScrHeight / 2;
	int nx = (p.cx + x * p.dx - y * p.dy) / 256;
	int ny = (p.cy + x * p.dy + y * p.dx) / 256;
	return nx ^ ny;
}

// ---------------------------------------------------------------------------------------
// plasma
// ---------------------------------------------------------------------------------------

template <typename T>
inline T slerp(T x, T b)
{
	return (3 * x * x * b - 2 * x * x * x) / (b * b);
}

template <typename T>
inline T fakesin(T x, T b)
{
	const T nx = x & (b - 1);
	const T nb = b / 2;
	return (nx < nb) ?  slerp<T>(nx, nb) : nb - slerp<T>(nx - nb, nb);
}

inline sf::Uint8 sample(int x, int y)
{
	return fakesin<int>(x, 256) + fakesin<int>(y, 256);
}

static inline sf::Uint8 computePlasma(int x, int y, const int& frame)
{
	const sf::Uint8 p0 = sample(x, y);
	const sf::Uint8 p1 = sample(x + fakesin<int>(y + (20 * frame) / 16, 256), y);
	const sf::Uint8 p2 = sample(x, y + fakesin<int>(x + (27 * frame) / 16, 256));
	return p0 + p1 + p2;
}

// ---------------------------------------------------------------------------------------
// fire
// ---------------------------------------------------------------------------------------
inline void setFire(sf::Uint16* d, int frame)
{
	// use 16bpp buffer to increase quality
	if (frame % 4 == 0)
	{
		for (int j = 0; j < ScrWidth;)
		{
			sf::Uint8 r = 192 + 63 * (rand() % 2);
			for (int i = 0; i < 10; ++i, ++j)
			{
				d[(ScrHeight - 1) * ScrWidth + j] = (r << 8) + rand() % 256;
			}
		}
	}
	for (int j = 0; j < 2; ++j)
	{
		for (int i = 0; i < (ScrHeight - 1) * ScrWidth; ++i)
		{
			d[i] = (2 * d[i] + 1 * d[i + ScrWidth - 1] + 3 * d[i + ScrWidth] + 2 * d[i + ScrWidth + 1]) / 8;
			if (d[i] > 255 /*&& d[i] < (200 << 8)*/)
				d[i] -= 256;
		}
	}
}

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
	auto plasma = new demo::buffer<sf::Uint8, ScrWidth, ScrHeight, demo::procst<sf::Uint8, ScrWidth, ScrHeight, int, computePlasma>>();

	// palettes
	auto palCC = demo::makePal<sf::Uint32, 256>([] (size_t i) {
		sf::Uint32 r = 0;
		if (demo::palRamp(0, 255, i, 0xff00ff00, 0xffff00ff, &r)) return r;
		return r;
	});
	auto palRZ = demo::makePal<sf::Uint32, 256>([] (size_t i) {
		sf::Uint32 r = 0;
		if (demo::palRamp(0, 255, i, 0xff0000ff, 0xffffff00, &r)) return r;
		return r;
	});
	auto palPlasma = demo::makePal<sf::Uint32, 256>([] (size_t i) {
		const sf::Uint32 c0 = 0xffff0000;
		const sf::Uint32 c1 = 0xff0000ff;
		const sf::Uint32 c2 = 0xff00ffff;
		sf::Uint32 r = 0;
		if (demo::palRamp(0  , 63 , i, c0, c1, &r)) return r;
		if (demo::palRamp(64 , 127, i, c1, c2, &r)) return r;
		if (demo::palRamp(128, 255, i, c2, c0, &r)) return r;
		return r;
	});
	auto palFire = demo::makePal<sf::Uint32, 256>([] (size_t i) {
		sf::Uint32 r = 0;
		if (demo::palRamp(0  , 63 , i, 0xff000000, 0xff0000ff, &r)) return r;
		if (demo::palRamp(64 , 127, i, 0xff0000ff, 0xff00ffff, &r)) return r;
		if (demo::palRamp(128, 191, i, 0xff00ffff, 0xffffffff, &r)) return r;
		if (demo::palRamp(192, 255, i, 0xffffffff, 0xffffffff, &r)) return r;
		return r;
	});

	// run loop
	win.run([&] (tWin320x200::tBackBuffer& bgFb, int frame) {
		const int fxDuration = 1000;
		if (frame < 1 * fxDuration)
		{
			plasma->_params = frame;
			bgFb.transformOfs(fb8->copyXY(*plasma), [&palPlasma] (sf::Uint8 l) { return palPlasma[l]; });
		}
		else if (frame < 2 * fxDuration)
		{
			const float rzf = 0.5f * frame;
			const float a = 0.03f * rzf;
			const float z = 1.2f + cosf(0.05f * rzf);
			const float r = 500.0f;
			rotozoom->_params = {
				int(128.0f + 256.0f * r * cosf(0.03f * rzf)) , int(128.0f + 256.0f * r * cosf(0.04f * rzf)),
				int(256.0f * z * cosf(a)), int(256.0f * z * sinf(a))
			};
			bgFb.transformOfs(fb8->copyXY(*rotozoom), [&palRZ] (sf::Uint8 l) { return palRZ[l]; });
		}
		else if (frame < 3 * fxDuration)
		{
			setFire(fb16->data(), frame);
			bgFb.transformOfs(*fb16, [&palFire] (sf::Uint16 l) { return palFire[l >> 8]; });
		}
		else if (frame < 4 * fxDuration)
		{
			cc->_params = {
				int(160 + 150 * sinf(0.03f * frame)), 100,
				160, int(100 + 90 * sinf(0.04f * frame))
			};
			bgFb.transformOfs(fb8->copyXY(*cc), [&palCC] (sf::Uint8 l) { return palCC[l]; });
		}
		else
		{
			return false;
		}
		return true;
	});
	
	return 0;
}

