#pragma once

#include <cmath>

#include <functional>

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>

#define SYNC_60Hz 1

namespace demo
{

template <typename T, int W, int H>
class frameb
{
public:
	static constexpr int LENGTH = W * (H + 1);
	template <typename... P>
	void init(const std::function<T (int, int, P...)>& f, P... p)
	{
		for (int y = 0, offset = 0; y < H; ++y)
			for (int x = 0; x < W; ++x)
				_data[offset++] = f(x, y, p...);
	}
	void fill(T v)
	{
		for (int i = 0; i < LENGTH; ++i)
			_data[i] = v;
	}
	T& xy(int x, int y) { return _data[y * W + x]; }
	T& ofs(int o) { return _data[o]; }
	const T& xy(int x, int y) const { return _data[y * W + x]; }
	const T& ofs(int o) const { return _data[o]; }
	T* data() { return _data; }
private:
	T _data[LENGTH];
};

template <typename T, int W, int H, T (*F)(int, int, int, int)>
class procfn
{
public:
	T xy(int x, int y) const { return F(W, H, x, y); }
	T ofs(int o) const { return F(W, H, o % W, o / W); }
};

template <typename T, int W, int H, class P, T (*F)(int, int, int, int, const P&)>
class procst
{
public:
	P _params;
	T xy(int x, int y) const { return F(W, H, x, y, _params); }
	T ofs(int o) const { return F(W, H, o % W, o / W, _params); }
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
	typedef std::function<bool (tBackBuffer&, int, bool&)> tRunFunc;

	demowin()
	{
	}

	template <class T>
	buffer<T, W, H, demo::frameb<T, W, H>>* createBuffer()
	{
		return new buffer<T, W, H, demo::frameb<T, W, H>>();
	}

	void run(const tRunFunc& f)
	{
		auto bgFb = new tBackBuffer();

		sf::RenderWindow win(sf::VideoMode(W, H), "toto");
#if SYNC_60Hz
		win.setFramerateLimit(60);
		win.setVerticalSyncEnabled(true);
#endif

		sf::Texture bg;
		bg.create(W, H);
		sf::Sprite bgSp(bg);

		sf::View view = win.getDefaultView();
		sf::Vector2u winSize(W, H);

		int screenIdx = 0;
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

			bool screenShot = false;
			if (!f(*bgFb, frame, screenShot)) {
				win.close();
				break;
			}

			bg.update((sf::Uint8*)&bgFb->ofs(0));

			if (screenShot) {
				char buffer[256] = {};
				snprintf(buffer, 256, "fx%04d.png", screenIdx);
				++screenIdx;
				auto img = bg.copyToImage();
				img.saveToFile(buffer);
			}

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

}

