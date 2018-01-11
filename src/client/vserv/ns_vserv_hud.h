#ifndef _VSERV_HUD_H_
#define _VSERV_HUD_H_

#include <stdexcept>

#include <client/renderingengine.h>
#include <irrlichttypes_extrabloated.h>

class VServHud
{
public:

	VServHud():
		m_driver(RenderingEngine::get_video_driver()),
		m_texture(NULL)
	{
		if (! m_driver)
			throw std::runtime_error("VServ hud");
	}

	void remakeTexture()
	{
		core::dimension2d<u32> dim(m_spectr_width, m_spectr_height);
		video::IImage *spectr_image = m_driver->createImage(video::ECF_A8R8G8B8, dim);

		video::SColor greencolor(255, 0, 255, 0);

		for (u16 y = 0; y < m_spectr_height; y++)
		for (u16 x = 0; x < m_spectr_width; x++) {
			spectr_image->setPixel(x, y, greencolor);
		}

		if (m_texture)
			m_driver->removeTexture(m_texture);
		m_texture = m_driver->addTexture("spectr__", spectr_image);
	}

	void drawSpectr()
	{
		remakeTexture();

		m_driver->draw2DImage(m_texture, core::vector2di(0, 0));
	}

private:
	video::IVideoDriver * m_driver = NULL;
	video::ITexture     * m_texture = NULL;

	u16 m_spectr_width  = 640;
	u16 m_spectr_height = 128;
};

#endif /* _VSERV_HUD_H_ */
