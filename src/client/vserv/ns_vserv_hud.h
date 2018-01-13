#ifndef _VSERV_HUD_H_
#define _VSERV_HUD_H_

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

		HudScroll(size_t width, size_t height, size_t uniq_texname_id) :
			m_driver(RenderingEngine::get_video_driver()),
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

			m_driver->draw2DImage(tex_r, core::vector2di(m_imgpos, 0), core::recti(0, 0, m_dim.Width - m_imgpos, m_dim.Height));
			m_driver->draw2DImage(tex_l, core::vector2di(0, 0), core::recti(m_dim.Width - m_imgpos, 0, m_dim.Width, m_dim.Height));

			scrollPostAdjust();
		}

		//for (u16 y = 0; y < m_dim.Height; y++)
		//	for (u16 x = 0; x < scrollby; x++) {
		//		size_t drawbase = m_dim.Width - m_imgpos;
		//		img_r->setPixel(drawbase + x, y, video::SColor(255, MYMIN(64 + (drawbase + x) / 3, 255), MYMIN(64 + (drawbase + x) / 3, 255), 0));
		//}
		virtual void virtualScrollTextureHelper(size_t scrollby, size_t drawbase, video::IImage *img_r)
		{
			for (u16 y = 0; y < m_dim.Height; y++)
			for (u16 x = 0; x < scrollby; x++) {
				img_r->setPixel(drawbase + x, y, video::SColor(255, MYMIN(64 + (drawbase + x) / 3, 255), MYMIN(64 + (drawbase + x) / 3, 255), 0));
			}
		}

		virtual size_t virtualScrollByHelper()
		{
			return 16;
		}

	private:
		video::IVideoDriver * m_driver;

		core::dimension2d<u32> m_dim;
		std::string       m_name[2];
		video::IImage   * m_image[2];
		video::ITexture * m_texture[2];
		size_t m_imgidx;
		size_t m_imgpos;
	};

	VServHud() :
		m_driver(RenderingEngine::get_video_driver()),
		m_dim(640, 128),
		m_scroll(new HudScroll(m_dim.Width, m_dim.Height, 0)),
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
