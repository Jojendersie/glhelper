#include "buffer.hpp"
#include "utils/flagoperators.hpp"

namespace gl
{

    Buffer::Buffer( std::uint32_t _sizeInBytes, Usage _usageFlags, const void* _data ) :
        m_sizeInBytes(_sizeInBytes),
		m_glMapAccess(0),
		m_usageFlags(_usageFlags),

		m_mappedDataOffset(0),
		m_mappedDataSize(0),
		m_mappedData(nullptr)
    {
		GLHELPER_ASSERT(static_cast<uint32_t>(m_usageFlags & Usage::EXPLICIT_FLUSH) == 0 || static_cast<uint32_t>(m_usageFlags & Usage::MAP_PERSISTENT) > 0,
			   "EXPLICIT_FLUSH only valid in combination with PERSISTENT");
		GLHELPER_ASSERT(static_cast<uint32_t>(m_usageFlags & Usage::MAP_COHERENT) == 0 || static_cast<uint32_t>(m_usageFlags & Usage::MAP_PERSISTENT) > 0,
			   "MAP_COHERENT only valid in combination with PERSISTENT");

		if(glCreateBuffers)
		{
			GL_CALL(glCreateBuffers, 1, &m_bufferObject);
			GL_CALL(glNamedBufferStorage, m_bufferObject, _sizeInBytes, _data, static_cast<GLbitfield>(_usageFlags));
		} else {
			GL_CALL(glGenBuffers, 1, &m_bufferObject);
			GL_CALL(glBindBuffer, GL_SHADER_STORAGE_BUFFER, m_bufferObject);
			GL_CALL(glBufferStorage, GL_SHADER_STORAGE_BUFFER, _sizeInBytes, _data, static_cast<GLbitfield>(_usageFlags));
		}

		if (any(m_usageFlags & Usage::MAP_READ))
			m_glMapAccess = static_cast<uint32_t>(m_usageFlags & Usage::MAP_WRITE) > 0 ? GL_READ_WRITE : GL_READ_ONLY;
        else
			m_glMapAccess = static_cast<uint32_t>(m_usageFlags & Usage::MAP_WRITE) > 0 ? GL_WRITE_ONLY : 0;


		if (any(m_usageFlags & Usage::MAP_PERSISTENT))
		{
			m_glMapAccess |= GL_MAP_PERSISTENT_BIT;
			if (static_cast<uint32_t>(m_usageFlags & Usage::EXPLICIT_FLUSH) > 0)
				m_glMapAccess |= GL_MAP_FLUSH_EXPLICIT_BIT;
			if (static_cast<uint32_t>(m_usageFlags & Usage::MAP_COHERENT) > 0)
				m_glMapAccess |= GL_MAP_COHERENT_BIT;

			// Persistent buffers do not need to be unmapped!
			Map();
		}
    }

    Buffer::~Buffer()
    {
		if (m_bufferObject != 0xffffffff)
		{
			if (m_mappedData)
			{
				if(glUnmapNamedBuffer)
					GL_CALL(glUnmapNamedBuffer, m_bufferObject);
				else {
					GL_CALL(glBindBuffer, GL_SHADER_STORAGE_BUFFER, m_bufferObject);
					GL_CALL(glUnmapBuffer, GL_SHADER_STORAGE_BUFFER);
				}
				m_mappedData = nullptr;
			}
			GL_CALL(glDeleteBuffers, 1, &m_bufferObject);
		}
    }

	Buffer::Buffer( Buffer&& _rValue ) :
		m_bufferObject(_rValue.m_bufferObject),
		m_sizeInBytes(_rValue.m_sizeInBytes),
		m_glMapAccess(_rValue.m_glMapAccess),
		m_usageFlags(_rValue.m_usageFlags)
	{
		_rValue.m_bufferObject = 0xffffffff;
	}

    void* Buffer::Map( std::uint32_t _offset, std::uint32_t _numBytes )
    {
        if (m_glMapAccess == 0)
            LOG_ERROR( "The buffer was not created with READ or WRITE flag. Unable to map memory!" );
		else
		{
			if (m_mappedData != nullptr)
			{
				if (m_mappedDataOffset <= _offset && m_mappedDataOffset + m_mappedDataSize >= _offset + _numBytes)
					return static_cast<char*>(m_mappedData) + _offset;
				else
				{
					LOG_WARNING("Buffer was already mapped, but within incompatible range. Performing Buffer::Unmap ...");
					Unmap();
				}
			}

			if (m_mappedData == nullptr) // (still) already mapped?
			{
				if(glMapNamedBufferRange)
					m_mappedData = GL_RET_CALL(glMapNamedBufferRange, m_bufferObject, _offset, _numBytes, static_cast<GLbitfield>(m_usageFlags & ~Usage::SUB_DATA_UPDATE));
				else {
					GL_CALL(glBindBuffer, GL_SHADER_STORAGE_BUFFER, m_bufferObject);
					m_mappedData = GL_RET_CALL(glMapBufferRange, GL_SHADER_STORAGE_BUFFER, _offset, _numBytes, static_cast<GLbitfield>(m_usageFlags & ~Usage::SUB_DATA_UPDATE));
				}
				m_mappedDataOffset = _offset;
				m_mappedDataSize = _numBytes;
			}
		}
		
        return m_mappedData;
    }

    void Buffer::Unmap()
	{
		if (m_mappedData == nullptr)
		{
			LOG_WARNING("Buffer is not mapped, ignoring unmap operation!");
		}

		// Persistent mapped buffers need no unmapping
		else if (any(m_usageFlags & Usage::MAP_PERSISTENT))
		{
			LOG_WARNING("Buffer has MAP_PERSISTENT flag and no EXPLICIT_FLUSH flag, unmaps are withouat any effect!");
		}
		else
		{
			if(glUnmapNamedBuffer)
				GL_CALL(glUnmapNamedBuffer, m_bufferObject);
			else {
				GL_CALL(glBindBuffer, GL_SHADER_STORAGE_BUFFER, m_bufferObject);
				GL_CALL(glUnmapBuffer, GL_SHADER_STORAGE_BUFFER);
			}
			m_mappedData = nullptr;
			m_mappedDataOffset = 0;
			m_mappedDataSize = 0;
		}
    }

	void Buffer::Flush()
	{
		Flush(m_mappedDataOffset, m_mappedDataSize);
	}

	void Buffer::Flush(std::uint32_t _offset, std::uint32_t _numBytes)
	{
		if (any(m_usageFlags & Usage::EXPLICIT_FLUSH))
		{
			// Flush only the part which was used.
			if(glFlushMappedNamedBufferRange)
				GL_CALL(glFlushMappedNamedBufferRange, m_bufferObject, _offset, _numBytes);
			else {
				GL_CALL(glBindBuffer, GL_SHADER_STORAGE_BUFFER, m_bufferObject);
				GL_CALL(glFlushMappedBufferRange, GL_SHADER_STORAGE_BUFFER, _offset, _numBytes);
			}
		}
	}


	void Buffer::Set(const void* _data, std::uint32_t _numBytes, std::uint32_t _offset)
    {
		if (any(m_usageFlags & Usage::SUB_DATA_UPDATE))
			LOG_ERROR("The buffer was not created with the SUB_DATA_UPDATE flag. Unable to set memory!");
		else if (m_mappedData != NULL && (static_cast<GLenum>(m_usageFlags & Usage::MAP_PERSISTENT)))
			LOG_ERROR("Unable to set memory for currently mapped buffer that was created without the PERSISTENT flag.");
		else {
			if(glNamedBufferSubData)
				GL_CALL(glNamedBufferSubData, m_bufferObject, _offset, _numBytes, _data);
			else {
				GL_CALL(glBindBuffer, GL_SHADER_STORAGE_BUFFER, m_bufferObject);
				GL_CALL(glBufferSubData, GL_SHADER_STORAGE_BUFFER, _offset, _numBytes, _data);
			}
		}
    }

	void Buffer::Get(void* _data, std::uint32_t _offset, std::uint32_t _numBytes)
    {
		if (any(m_usageFlags & Usage::SUB_DATA_UPDATE))
			LOG_ERROR("The buffer was not created with the SUB_DATA_UPDATE flag. Unable to get memory!");
		else if (m_mappedData != NULL && (static_cast<GLenum>(m_usageFlags & Usage::MAP_PERSISTENT)))
			LOG_ERROR("Unable to get memory for currently mapped buffer that was created without the PERSISTENT flag.");
		else {
			if(glGetNamedBufferSubData)
				GL_CALL(glGetNamedBufferSubData, m_bufferObject, _offset, _numBytes, _data);
			else {
				GL_CALL(glBindBuffer, GL_SHADER_STORAGE_BUFFER, m_bufferObject);
				GL_CALL(glGetBufferSubData, GL_SHADER_STORAGE_BUFFER, _offset, _numBytes, _data);
			}
		}
    }
}