#include "texturecubemap.hpp"

#ifdef TEXTURE_FROMFILE_STBI
// The stb_migle implementation is included in texture2d.cpp!
#include <stb_image.h>
#endif

namespace gl
{
	TextureCubemap::TextureCubemap(GLsizei _sideLength, TextureFormat format, GLsizei _numCubemaps, GLsizei numMipLevels) :
		Texture(_sideLength, _sideLength, 6 * _numCubemaps, format, numMipLevels)
	{
		GL_CALL(glCreateTextures, GL_TEXTURE_CUBE_MAP_ARRAY, 1, &m_textureHandle);
		GL_CALL(glTextureStorage3D, m_textureHandle, m_numMipLevels, gl::TextureFormatToGLSizedInternal[static_cast<unsigned int>(format)], m_width, m_height, m_depth);
	}

	void TextureCubemap::SetData(GLsizei _index, Face _face, GLsizei _mipLevel, TextureSetDataFormat _dataFormat, TextureSetDataType _dataType, const void* _data)
	{
		GLHELPER_ASSERT(_mipLevel >= 0, "Invalid mipmap level");
		GLHELPER_ASSERT(_mipLevel < m_numMipLevels, "MipLevel " + std::to_string(_mipLevel) + " does not exist, texture has only " + std::to_string(m_numMipLevels) + " MipMapLevels");

		GL_CALL(glTextureSubImage3D, m_textureHandle, _mipLevel, 0, 0,
					_index * 6 + static_cast<int>(_face), m_width, m_height, 1,
					static_cast<GLenum>(_dataFormat), static_cast<GLenum>(_dataType), _data);
	}

#ifdef TEXTURE_FROMFILE_STBI
	bool TextureCubemap::LoadFaceFromFile(GLsizei _index, Face _face, const std::string& _filename)
	{
		int texSizeX = -1;
		int texSizeY = -1;

		int numComps = -1;
		stbi_uc* textureData = stbi_load(_filename.c_str(), &texSizeX, &texSizeY, &numComps, 3);
		if (!textureData)
		{
			GLHELPER_LOG_ERROR("Error loading texture \"" + _filename + "\".");
			return false;
		}
		if (texSizeX != texSizeY || texSizeX != m_width)
		{
			GLHELPER_LOG_ERROR("Texture \"" + _filename + "\" has an invalid size to be used as cubemap. WxH is " + std::to_string(texSizeX) + 'x' + std::to_string(texSizeY) + " but must be " + std::to_string(m_width) + ".");
			stbi_image_free(textureData);
			return false;
		}

		SetData(_index, _face, 0, TextureSetDataFormat::RGB, TextureSetDataType::UNSIGNED_BYTE, textureData);

		stbi_image_free(textureData);

		return true;
	}
#endif
}