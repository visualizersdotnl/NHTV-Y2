
/*
	D3D: buffer wrappers.
*/

#pragma once

namespace D3D
{
	class Buffer : public boost::noncopyable
	{
	public:
		typedef std::unique_ptr<Buffer> Ptr;

		Buffer(ID3D11Buffer &buffer, size_t size) :
			m_pBuffer(&buffer), m_size(size)
		{
		}

		virtual ~Buffer()
		{
			SAFE_RELEASE(m_pBuffer);
		}

		// Always upload the entire buffer; this suffices in most designs and is most hardware-friendly.
		void Upload(const void* data, size_t numBytes)
		{
			// I figure this mistake will be made a couple of times, better catch it here.
			ASSERT(m_size == numBytes);

			D3D11_MAPPED_SUBRESOURCE mappedRes;
			VERIFY(S_OK == GetContext()->Map(m_pBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedRes));
			{
				memcpy(mappedRes.pData, data, m_size);
			}
			GetContext()->Unmap(m_pBuffer, 0);
		}

		ID3D11Buffer *Get() const { return m_pBuffer; }
		operator ID3D11Buffer *() const { return Get(); }

		size_t GetSize() const { return m_size; }

		// Map() and Unmap() take an ID3D11Resource pointer, so use Get() for those calls.
		// I can't reasonably cast to this without it being confusing and bad design.

	private:
		ID3D11Buffer *m_pBuffer;
		size_t m_size;
	};

	class VertexBuffer : public Buffer
	{
	public:
		typedef std::unique_ptr<VertexBuffer> Ptr;

		VertexBuffer(ID3D11Buffer &buffer, size_t numBytes) :
			Buffer(buffer, numBytes)
		{
		}

		~VertexBuffer() {}

	private:
	};

	class IndexBuffer : public Buffer
	{
	public:
		typedef std::unique_ptr<IndexBuffer> Ptr;

		IndexBuffer(ID3D11Buffer &buffer, size_t numBytes) :
			Buffer(buffer, numBytes)
		{
		}

		~IndexBuffer() {}

	private:
	};

	class ConstantBufferGPU : public Buffer
	{
	public:
		typedef std::unique_ptr<ConstantBufferGPU> Ptr;

		ConstantBufferGPU(ID3D11Buffer &buffer, size_t numBytes, const std::string &name) :
			Buffer(buffer, numBytes)
			, m_name(name)
		{
			// Must be a multiple of 16 bytes (128-bit vector).
			ASSERT(0 == (numBytes & 15));
		}

		virtual ~ConstantBufferGPU() {}

		void Upload(const void *data, size_t numBytes)
		{
			Buffer::Upload(data, numBytes);
		}

		const std::string &GetName() const { return m_name; }

	private:
		const std::string &m_name;
	};

	// This is a workaround to keep ConstantBufferGPU non-templatized and easy to pass around.
	template <typename T>
	class ConstantBuffer : public ConstantBufferGPU
	{
	public:
		typedef std::unique_ptr<ConstantBuffer> Ptr;

		ConstantBuffer(ID3D11Buffer &buffer, const std::string &name) :
			ConstantBufferGPU(buffer, sizeof(T), name)
		{
			memset(&m_local, 0, sizeof(T));
		}

		~ConstantBuffer() {}

		void Upload()
		{
			ConstantBufferGPU::Upload(&m_local, sizeof(T));
		}

		T m_local;
	};
}
