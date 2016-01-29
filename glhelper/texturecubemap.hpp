#pragma once

#include "texture.hpp"
#include <memory>
#include <string>

namespace gl
{
	/// Cubemap texture array. There is no separate implementation for single
	/// cubemaps since an array with a single element can be used as well.
	/// In shader you need to use the |g|samplerCubeArray[Shadow] sampler variants.
	class TextureCubemap : public Texture
	{
	public:
		enum class Face {
			POSITIVE_X = 0,
			NEGATIVE_X = 1,
			POSITIVE_Y = 2,
			NEGATIVE_Y = 3,
			POSITIVE_Z = 4,
			NEGATIVE_Z = 5
		};

		/// New empty cubemap texture.
		///
		/// \param _numCubemaps
		///		The cubemap texture is an array by default. To allocate a single cubemap use
		///		a value of 1.
		/// \param _numMipLevels
		///		Number of MipMap levels. If 0, the maximum possible number of mipmaps will be determined and used.
		TextureCubemap(GLsizei _sideLength, TextureFormat _format, GLsizei _numCubemaps = 1, GLsizei _numMipLevels = 1);

		/// Overwrites all data of a given mip level.
		///
		/// \param _index
		///		Array index for the cubemap. For single cubemaps this is always 0.
		void SetData(GLsizei _index, Face _face, GLsizei _mipLevel, TextureSetDataFormat _dataFormat, TextureSetDataType _dataType, const void* _data);

		GLenum GetOpenGLTextureType() const override { return GL_TEXTURE_CUBE_MAP; }

#ifdef TEXTURE_FROMFILE_STBI
		/// \brief Loads a 2D texture from file into a single layer using stb_image
		///
		/// TODO: you might want have mipmap generation support...
		bool LoadFaceFromFile(GLsizei _index, Face _face, const std::string& _filename);
#endif
	};

}

