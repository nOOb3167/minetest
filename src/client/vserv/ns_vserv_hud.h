#ifndef _VSERV_HUD_H_
#define _VSERV_HUD_H_

#include <deque>
#include <mutex>
#include <stdexcept>

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

	VServHud() :
		m_driver(RenderingEngine::get_video_driver()),
		m_dim(640, 128),
		m_image   { NULL, NULL },
		m_texture { NULL, NULL },
		m_imgidx(0),
		m_imgpos(m_dim.Width),
		m_queue_msg(),
		m_queue_mutex()
	{
		if (! m_driver)
			throw std::runtime_error("VServ hud");

		for (size_t i = 0; i < 2; i++)
			m_image[i] = m_driver->createImage(video::ECF_A8R8G8B8, m_dim);

		if (! m_image[0] || ! m_image[1])
			throw std::runtime_error("VServ hud");

		for (size_t i = 0; i < 2; i++)
			m_texture[i] = m_driver->addTexture(m_name[i], m_image[i]);

		if (! m_texture[0] || ! m_texture[1])
			throw std::runtime_error("VServ hud");
	}

	~VServHud()
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
		video::ITexture *& tex_r = m_texture[m_imgidx];
		video::ITexture *& tex_l = m_texture[(m_imgidx + 1) % 2];
		const char *tex_r_name = m_name[m_imgidx];

		m_imgpos = MYMAX(0, m_imgpos - scrollby);

		for (u16 y = 0; y < m_dim.Height; y++)
		for (u16 x = 0; x < scrollby ; x++) {
			img_r->setPixel(m_imgpos + x, y, video::SColor(255, (m_imgpos + x)/3, (m_imgpos + x)/3, 0));
		}

		m_driver->removeTexture(tex_r);
		tex_r = m_driver->addTexture(tex_r_name, img_r);
		if (! tex_r)
			throw std::runtime_error("VServ hud tex");

		if (m_imgpos == 0) {
			m_imgpos = m_dim.Width;
			m_imgidx = (m_imgidx + 1) % 2;
		}
	}

	void drawSpectr()
	{
		scrollTexture(16);

		video::ITexture *& tex_r = m_texture[m_imgidx];
		video::ITexture *& tex_l = m_texture[(m_imgidx + 1) % 2];

		m_driver->draw2DImage(tex_r, core::vector2di(m_imgpos, 0), core::recti(0, 0, m_dim.Width - m_imgpos, m_dim.Height));
		m_driver->draw2DImage(tex_l, core::vector2di(0, 0), core::recti(m_dim.Width - m_imgpos, 0, m_dim.Width, m_dim.Height));
	}

private:
	video::IVideoDriver * m_driver = NULL;

	core::dimension2d<u32> m_dim;
	video::IImage   * m_image[2];
	video::ITexture * m_texture[2] = { NULL, NULL };
	const char      * m_name[2]    = { "vserv_hud_spectr_0__", "vserv_hud_spectr_1__" };
	size_t m_imgidx;
	size_t m_imgpos;

	std::deque<HudMsg> m_queue_msg;
	std::mutex         m_queue_mutex;
};

#endif /* _VSERV_HUD_H_ */
