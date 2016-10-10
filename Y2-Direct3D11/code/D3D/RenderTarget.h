
/*
	D3D: render target wrapper.
*/

#pragma once

namespace D3D
{
	class RenderTarget : public boost::noncopyable
	{
	public:
		typedef std::unique_ptr<RenderTarget> Ptr;

		RenderTarget(DXGI_FORMAT format, ID3D11Texture2D *pTexture, ID3D11RenderTargetView *pTargetView, ID3D11ShaderResourceView *pShaderView) :
			format(format)
			, m_pTexture(pTexture), m_pTargetView(pTargetView), m_pShaderView(pShaderView)
		{
			ASSERT(pTexture != nullptr);
			ASSERT(pTargetView != nullptr);
//			ASSERT(pShaderView != nullptr);
		}

		~RenderTarget()
		{
			SAFE_RELEASE(m_pTexture);
			SAFE_RELEASE(m_pTargetView);
			SAFE_RELEASE(m_pShaderView);
		}

		void ResolveTo(RenderTarget& target)
		{
			ASSERT(nullptr != target.GetTexture() && nullptr != m_pTexture);
			GetContext()->ResolveSubresource(target.GetTexture(), 0, m_pTexture, 0, format);
		}

		ID3D11Texture2D          *GetTexture()    const { return m_pTexture; }
		ID3D11RenderTargetView   *GetTargetView() const { return m_pTargetView; }
		ID3D11ShaderResourceView *GetShaderView() const { return m_pShaderView; }

	private:
		DXGI_FORMAT format;
		ID3D11Texture2D *m_pTexture;
		ID3D11RenderTargetView *m_pTargetView;
		ID3D11ShaderResourceView *m_pShaderView;
	};
}
