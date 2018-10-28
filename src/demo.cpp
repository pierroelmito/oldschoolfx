
#include "demohelper.hpp"

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
	const int dx = x1 - x0;
	const int dy = y1 - y0;
	return dx * dx + dy * dy;
}

static inline sf::Uint8 computeCC(int, int, int x, int y, const ccparams& p)
{
	const int u = dist(p.x0, p.y0, x, y) / 512;
	const int v = dist(p.x1, p.y1, x, y) / 512;
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

inline sf::Uint8 computeRotozoom(int w, int h, int x, int y, const rzparams& p)
{
	x -= w / 2;
	y -= h / 2;
	const int nx = (p.cx + x * p.dx - y * p.dy) / 256;
	const int ny = (p.cy + x * p.dy + y * p.dx) / 256;
	return demo::sample(p.data, nx, ny);
}

// ---------------------------------------------------------------------------------------
// plasma
// ---------------------------------------------------------------------------------------

inline sf::Uint8 samplePlasma(int x, int y)
{
	return demo::fakesin<int>(x, 256) + demo::fakesin<int>(y, 256);
}

struct plasmaparams
{
	sf::Uint8* data;
	int frame;
};

static inline sf::Uint8 computePlasma(int, int, int x, int y, const plasmaparams& p)
{
	const sf::Uint8 p0 = demo::sample(p.data, x + y / 2, 3 * y / 2);
	const sf::Uint8 p1 = demo::sample(p.data, x + demo::fakesin<int>(y + (20 * p.frame) / 16, 256), y);
	const sf::Uint8 p2 = demo::sample(p.data, 5 * x / 4, y + demo::fakesin<int>(x + (27 * p.frame) / 16, 256));
	return p0 + p1 + p2;
}

// ---------------------------------------------------------------------------------------
// fire
// ---------------------------------------------------------------------------------------
inline void setFire(int w, int h, sf::Uint16* d, int frame)
{
	// use 16bpp buffer to increase quality

	// update random values at bottom pixel line
	if (frame % 4 == 0)
	{
		for (int j = 0; j < w;)
		{
			sf::Uint8 r = 192 + 63 * (rand() % 2);
			// 10 pixels bloc
			for (int i = 0; i < 10; ++i, ++j)
				d[(h - 1) * w + j] = (r << 8) + rand() % 256;
		}
	}

	// two loops per frame for quicker fire
	for (int j = 0; j < 2; ++j)
	{
		for (int i = 0; i < (h - 1) * w; ++i)
		{
			// blur upward in place
			d[i] = (2 * d[i] + 1 * d[i + w - 1] + 3 * d[i + w] + 2 * d[i + w + 1]) / 8;
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
		const float rx1 = demo::slerpf(float(x - l) / float(r - l));
		const float ry1 = demo::slerpf(float(y - t) / float(b - t));
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

// ---------------------------------------------------------------------------------------
// bump
// ---------------------------------------------------------------------------------------

void bump(sf::Uint8* dst, const sf::Uint8* src, int W, int H, int lposx, int lposy)
{
	const int coeff = 16;
	const int div = 256;

	for (int y = 0, offset = 0; y < H; ++y)
	{
		dst[offset++] = 0;
		for (int x = 1; x < W; ++x, ++offset)
		{
			const int nx = int(src[offset]) - int(src[offset - 1]);
			const int ny = int(src[offset + W]) - int(src[offset]);
			const int lx = lposx - x + coeff * nx;
			const int ly = lposy - y + coeff * ny;
			const int sql = (lx * lx + ly * ly) / div;
			dst[offset] = sql > 255 ? 0 : 255 - sql;
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

void drawBars(sf::Uint8* dst, int W, int H, int frame)
{
	auto computeOfs = [&] (float scale, int a, int o[4]) {
		const float pidiv2 = 0.5f * 3.14f;
		const float rad = 4.0f * a * pidiv2 / 255.0f;
		const float br = scale;
		const float x[4] = {
			W / 2.0f + br * cosf(0.5f * pidiv2 - rad),
			W / 2.0f + br * cosf(1.5f * pidiv2 - rad),
			W / 2.0f + br * cosf(2.5f * pidiv2 - rad),
			W / 2.0f + br * cosf(3.5f * pidiv2 - rad),
		};
		int minIdx = 0;
		float minV = x[0];
		for (int i = 1; i < 4; ++i) {
			if (x[i] < minV) {
				minV = x[i];
				minIdx = i;
			}
		}
		o[0] = minIdx;
		o[1] = x[minIdx];
		o[2] = x[(minIdx + 1) % 4];
		o[3] = x[(minIdx + 2) % 4];
	};
	auto getColor = [&] (int f, int w) {
		return f * 64 + w / 2;
	};
	const float da0 = 300.0f * sinf((frame + 300) * 0.01f);
	const float da1 = 200.0f * sinf((frame + 100) * 0.04f);
	const int da = da0 + da1;
	for (int y = 0, offset = 0; y < H; ++y)
	{
		const float ds = 25.0f * sinf((y + frame) * 0.02f);
		const float dx0 = 16.0f * sinf((y + frame * 3) * 0.03f);
		const float dx1 = 8.0f * sinf((y + frame * 2) * 0.05f);
		const int dx = dx0 + dx1;
		int ofs[4];
		computeOfs(50.0f + ds, (frame * 50 + y * da) / H, ofs);
		const int f0 = ofs[0];
		const int f1 = (f0 + 1) % 4;
		const int c0 = getColor(f0 , ofs[2] - ofs[1]);
		const int c1 = getColor(f1 , ofs[3] - ofs[2]);
		int x = 0;
		for (; x < ofs[1] + dx; ++x, ++offset)
			dst[offset] = 0;
		for (; x < ofs[2] + dx; ++x, ++offset)
			dst[offset] = c0;
		for (; x < ofs[3] + dx; ++x, ++offset)
			dst[offset] = c1;
		for (; x < W; ++x, ++offset)
			dst[offset] = 0;
	}
}

// ---------------------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------------------

int main(int, char**)
{
	// open window
	const int ScrWidth = 320;
	const int ScrHeight = 200;
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
	const auto palGrey    = demo::makeRampPal<sf::Uint32, 256>( { 0xff000000, 0xffffffff } );
	const auto palBump    = demo::makeRampPal<sf::Uint32, 256>( { 0xff0f0000, 0xff00007f, 0xff00007f, 0xff7fffff, 0xff7f7fff } );
	const auto palCC      = demo::makeRampPal<sf::Uint32, 256>( { 0xff00ff00, 0xffff00ff } );
	const auto palRZ      = demo::makeRampPal<sf::Uint32, 256>( { 0xff0000ff, 0xffffff00 } );
	const auto palPlasma  = demo::makeRampPal<sf::Uint32, 256>( { 0xffff0000, 0xff0000ff, 0xff00ffff, 0xffff0000 } );
	const auto palFire    = demo::makeRampPal<sf::Uint32, 256>( { 0xff000000, 0xff0000ff, 0xff00ffff, 0xffffffff, 0xffffffff } );
	const auto palDistort = demo::makePal<sf::Uint32, 256>([] (int i) {
		int c = 16 + 2 * (i % 64);
		switch (i / 64) {
			case 0: return demo::abgr(255, c, c, 0);
			case 1: return demo::abgr(255, 255 - c, c, 0);
			case 2: return demo::abgr(255, 0, 255 - c, c);
			case 3: return demo::abgr(255, c, c, 255 - c);
		}
		return 0u;
	});

	typedef std::function<void(tWin320x200::tBackBuffer&, int)> tFxFunc;

	// water
	tFxFunc waterFunc = [&] (tWin320x200::tBackBuffer& bgFb, int frame) {
		if (frame == 0) {
			waterInit(fb16a->data(), ScrWidth, ScrHeight); 
			waterInit(fb16b->data(), ScrWidth, ScrHeight); 
		}
		const float sc = 0.03f;
		const bool b = (frame % 2 == 0);
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
		const float sc = 0.03f;
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
		bgFb.transformXY(*plasma, [&] (sf::Uint8 l) { return palPlasma[l]; });
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
		bgFb.transformXY(*rotozoom, [&] (sf::Uint8 l) { return palRZ[l]; });
	};

	// fire
	tFxFunc fireFunc = [&] (tWin320x200::tBackBuffer& bgFb, int frame) {
		if (frame == 0) {
			fb16a->fill(0);
		}
		setFire(ScrWidth, ScrHeight, fb16a->data(), frame);
		bgFb.transformOfs(*fb16a, [&] (sf::Uint16 l) { return palFire[l >> 8]; });
	};

	// circles
	tFxFunc ccFunc = [&] (tWin320x200::tBackBuffer& bgFb, int frame) {
		cc->_params = {
			mito->data(),
			int(160 + 150 * sinf(0.03f * frame)), 100, // first pos
			160, int(100 + 90 * sinf(0.04f * frame)),  // second pos
		};
		bgFb.transformXY(*cc, [&] (sf::Uint8 l) { return palCC[l]; });
	};


	// bars
	tFxFunc barsFunc = [&] (tWin320x200::tBackBuffer& bgFb, int frame) {
		drawBars(fb8a->data(), ScrWidth, ScrHeight, frame);
		bgFb.transformOfs(*fb8a, [&] (sf::Uint8 l) { return palDistort[l]; });
	};

	// FX list
	tFxFunc fxs[] = {
		waterFunc,
		bumpFunc,
		plasmaFunc,
		rzFunc,
		ccFunc,
		fireFunc,
		barsFunc,
	};

	// run function
	tWin320x200::tRunFunc runFunc = [&] (tWin320x200::tBackBuffer& bgFb, int frame, bool& screenShot) {
		const int fxCount = sizeof(fxs) / sizeof(fxs[0]);
		const int fxDuration = 1500;
		if (frame >= fxDuration * fxCount)
			return false;
		const int fxIdx = frame / fxDuration;
		const int frameIdx = frame % fxDuration;
		if (frameIdx == fxDuration / 2)
			screenShot = true;
		fxs[fxIdx](bgFb, frameIdx);
		return true;
	};

	// run loop
	win.run(runFunc);

	return 0;
}

