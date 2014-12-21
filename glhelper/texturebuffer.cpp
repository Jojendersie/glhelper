﻿#include "texturebuffer.hpp"
#include "shaderobject.hpp"

namespace gl
{
	const TextureBufferView* TextureBufferView::s_boundTBOs[16];

	TextureBufferView::TextureBufferView() :
		m_textureObject(0)
	{
		if(glCreateTextures)
			GL_CALL(glCreateTextures, GL_TEXTURE_BUFFER, 1, &m_textureObject);
		else
			GL_CALL(glGenTextures, 1, &m_textureObject);
	}

	TextureBufferView::~TextureBufferView()
	{
        GL_CALL(glDeleteTextures, 1, &m_textureObject);
	}

	Result TextureBufferView::Init(std::shared_ptr<Buffer> _buffer, TextureBufferFormat _format)
	{
		return Init(_buffer, _format, 0, _buffer->GetSize());
	}

	Result TextureBufferView::Init(std::shared_ptr<Buffer> _buffer, TextureBufferFormat _format, std::uint32_t _offset, std::uint32_t _numBytes)
    {
        m_buffer = _buffer;
		if(glTextureBuffer)
		{
			GL_CALL(glTextureBufferRange, m_textureObject, 
				static_cast<GLenum>(_format), _buffer->GetBufferId(), _offset, _numBytes);
		} else {
			BindBuffer(0);
			GL_CALL(glTexBufferRange, GL_TEXTURE_BUFFER, 
				static_cast<GLenum>(_format), _buffer->GetBufferId(), _offset, _numBytes);
		}

        return Result::SUCCEEDED;
    }

	void TextureBufferView::BindBuffer(GLuint _locationIndex) const
	{
		GLHELPER_ASSERT(_locationIndex < sizeof(s_boundTBOs) / sizeof(TextureBufferView*), 
			"Can't bind tbo to slot " + std::to_string(_locationIndex) + ". Maximum number of slots is " + std::to_string(sizeof(s_boundTBOs) / sizeof(TextureBufferView*)));

		if (s_boundTBOs[_locationIndex] != this)
		{
            GL_CALL(glActiveTexture, GL_TEXTURE0 + _locationIndex); 
			GL_CALL(glBindTexture, GL_TEXTURE_BUFFER, m_textureObject);
			s_boundTBOs[_locationIndex] = this;
		}

	}
}