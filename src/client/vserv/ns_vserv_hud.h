#ifndef _VSERV_HUD_H_
#define _VSERV_HUD_H_

#include <climits>
#include <cstdint>
#include <deque>
#include <mutex>
#include <stdexcept>
#include <string>

#include <client/renderingengine.h>
#include <irrlichttypes_extrabloated.h>
#include <threading/mutex_auto_lock.h>
#include <util/basic_macros.h>

class VServHud
{
public:
	class HudMsg
	{
	public:
		std::string fra_buf;
	};

	class HudScroll
	{
	public:

		HudScroll(size_t x, size_t y, size_t width, size_t height, size_t uniq_texname_id) :
			m_driver(RenderingEngine::get_video_driver()),
			m_off(x, y),
			m_dim(width, height),
			m_name    {},
			m_image   { NULL, NULL },
			m_texture { NULL, NULL },
			m_imgidx(0),
			m_imgpos(m_dim.Width)
		{
			if (! m_driver)
				throw std::runtime_error("VServ hud scroll");

			m_name[0] = std::string("vserv_hud_spectr_").append(std::to_string(uniq_texname_id    )).append("__");
			m_name[1] = std::string("vserv_hud_spectr_").append(std::to_string(uniq_texname_id + 1)).append("__");

			for (size_t i = 0; i < 2; i++)
				m_image[i] = m_driver->createImage(video::ECF_A8R8G8B8, m_dim);

			if (! m_image[0] || ! m_image[1])
				throw std::runtime_error("VServ hud scroll");

			for (size_t i = 0; i < 2; i++)
				for (u16 y = 0; y < m_dim.Height; y++)
				for (u16 x = 0; x < m_dim.Width; x++)
					m_image[i]->setPixel(x, y, video::SColor(255, 0, 0, 0));

			for (size_t i = 0; i < 2; i++)
				m_texture[i] = m_driver->addTexture(m_name[i].c_str(), m_image[i]);

			if (! m_texture[0] || ! m_texture[1])
				throw std::runtime_error("VServ hud scroll");
		}

		~HudScroll()
		{
			for (size_t i = 0; i < 2; i++)
				if (m_texture[i])
					m_driver->removeTexture(m_texture[i]);

			for (size_t i = 0; i < 2; i++)
				if (m_image[i])
					m_image[i]->drop();
		}

		void scrollTexture(size_t scrollby)
		{
			video::IImage *img_r = m_image[m_imgidx];
			video::IImage *img_l = m_image[(m_imgidx + 1) % 2];
			/* texture referenced by tex_r will be rebound */
			video::ITexture *& tex_r = m_texture[m_imgidx];
			video::ITexture *  tex_l = m_texture[(m_imgidx + 1) % 2];
			std::string &tex_r_name = m_name[m_imgidx];

			scrollby = MYMIN(scrollby, m_imgpos);

			const size_t drawbase = m_dim.Width - m_imgpos;

			virtualScrollTextureHelper(scrollby, drawbase, img_r);

			m_imgpos -= scrollby;

			m_driver->removeTexture(tex_r);
			tex_r = m_driver->addTexture(tex_r_name.c_str(), img_r);
			if (!tex_r)
				throw std::runtime_error("VServ hud tex");
		}

		void scrollPostAdjust()
		{
			if (m_imgpos == 0) {
				m_imgpos = m_dim.Width;
				m_imgidx = (m_imgidx + 1) % 2;
			}
		}

		void draw()
		{
			const size_t scrollby = virtualScrollByHelper();

			if (scrollby)
				scrollTexture(scrollby);

			video::ITexture *& tex_r = m_texture[m_imgidx];
			video::ITexture *& tex_l = m_texture[(m_imgidx + 1) % 2];

			m_driver->draw2DImage(tex_r, m_off + core::vector2di(m_imgpos, 0), core::recti(0, 0, m_dim.Width - m_imgpos, m_dim.Height));
			m_driver->draw2DImage(tex_l, m_off + core::vector2di(0, 0), core::recti(m_dim.Width - m_imgpos, 0, m_dim.Width, m_dim.Height));

			scrollPostAdjust();
		}

		virtual void virtualScrollTextureHelper(size_t scrollby, size_t drawbase, video::IImage *img_r) = 0;

		virtual size_t virtualScrollByHelper() = 0;

	protected:
		video::IVideoDriver * m_driver;

		core::vector2di        m_off;
		core::dimension2d<u32> m_dim;
		std::string       m_name[2];
		video::IImage   * m_image[2];
		video::ITexture * m_texture[2];
		size_t m_imgidx;
		size_t m_imgpos;
	};

	class HudScrollYellow : public HudScroll
	{
	public:
		HudScrollYellow(size_t width, size_t height, size_t uniq_texname_id) :
			HudScroll(0, 0, width, height, uniq_texname_id)
		{}

		void virtualScrollTextureHelper(size_t scrollby, size_t drawbase, video::IImage *img_r) override
		{
			for (u16 y = 0; y < m_dim.Height; y++)
				for (u16 x = 0; x < scrollby; x++) {
					size_t drawbase = m_dim.Width - m_imgpos;
					img_r->setPixel(drawbase + x, y, video::SColor(255, MYMIN(64 + (drawbase + x) / 3, 255), MYMIN(64 + (drawbase + x) / 3, 255), 0));
			}
		}

		size_t virtualScrollByHelper() override
		{
			return 16;
		}
	};

	class HudScrollFrame : public HudScroll
	{
	public:
		HudScrollFrame(size_t x, size_t y, size_t width, size_t height, size_t uniq_texname_id, size_t scrollby) :
			HudScroll(x, y, width, height, uniq_texname_id),
			m_scrollby(scrollby),
			m_framedummy(GS_OPUS_FRAME_48KHZ_20MS_SAMP_NUM * sizeof(int16_t), '\0'),
			m_frames()
		{}

		inline long squeeze16(long val, long vertical_positive, long vertical_negative)
		{
			if (val > 0)
				return (float)(val * vertical_positive) / INT16_MAX;
			else
				return -((float)(val * vertical_negative) / INT16_MIN);
		}

		void drawFrame(size_t scrollby, size_t drawbase, video::IImage *img_r, const std::string &frame)
		{
			const size_t samples_num = GS_OPUS_FRAME_48KHZ_20MS_SAMP_NUM;
			assert(frame.size() == samples_num * sizeof(int16_t));

			const size_t samples_per_px = samples_num / scrollby;  // truncating division - ignore last samples_num % scrollby

			const int16_t *samples = (const int16_t *) frame.data();
			const int16_t *cursamp = samples;

			for (size_t x = 0; x < scrollby; x++) {
				long max = INT16_MIN;
				long min = INT16_MAX;
				size_t num_positive = 0;
				size_t num_negative = 0;
				float avg_positive = 0.0f;
				float avg_negative = 0.0f;
				for (size_t samp = 0; samp < samples_per_px; samp++, cursamp++) {
					max = MYMAX(max, *cursamp);
					min = MYMIN(min, *cursamp);
					if (*cursamp > 0) {
						num_positive++;
						avg_positive += *cursamp;
					}
					else {
						num_negative++;
						avg_negative += *cursamp;
					}
				}
				avg_positive /= num_positive ? num_positive : 1;
				avg_negative /= num_negative ? num_negative : 1;
				const size_t vertical_center_px = m_dim.Height / 2; // truncating division
				const size_t vertical_positive = m_dim.Height - vertical_center_px;
				const size_t vertical_negative = m_dim.Height - vertical_positive;
				const long yfrom = squeeze16(avg_negative, vertical_positive, vertical_negative);
				const long yto   = squeeze16(avg_positive, vertical_positive, vertical_negative);
				assert(yfrom <= yto);
				for (long y = yfrom; y <= yto; y++) // <=
					img_r->setPixel(drawbase + x, vertical_center_px + y, video::SColor(255, 255, 255, 0));
				img_r->setPixel(drawbase + x, vertical_center_px + squeeze16(max, vertical_positive, vertical_negative), video::SColor(255, 255, 0, 0));
				img_r->setPixel(drawbase + x, vertical_center_px + squeeze16(min, vertical_positive, vertical_negative), video::SColor(255, 255, 0, 0));
			}
		}

		void virtualScrollTextureHelper(size_t scrollby, size_t drawbase, video::IImage *img_r) override
		{
			if (m_frames.empty()) {
				drawFrame(scrollby, drawbase, img_r, m_framedummy);
			}
			else {
				drawFrame(scrollby, drawbase, img_r, m_frames.front());
				m_frames.pop_front();
			}
		}

		size_t virtualScrollByHelper() override
		{
			return m_scrollby;
		}

	private:
		size_t m_scrollby;
		std::string m_framedummy;
		std::deque<std::string> m_frames;
	};

	VServHud() :
		m_driver(RenderingEngine::get_video_driver()),
		m_dim(640, 128),
		//m_scroll(new HudScrollYellow(m_dim.Width, m_dim.Height, 0)),
		m_scroll(new HudScrollFrame(0, 0, m_dim.Width, m_dim.Height, 0, 16)),
		m_queue_msg(),
		m_queue_mutex()
	{}

	void drawSpectr()
	{
		m_scroll->draw();
	}

private:
	video::IVideoDriver * m_driver = NULL;

	core::dimension2d<u32>     m_dim;
	std::unique_ptr<HudScroll> m_scroll;

	std::deque<HudMsg> m_queue_msg;
	std::mutex         m_queue_mutex;
};

#endif /* _VSERV_HUD_H_ */
