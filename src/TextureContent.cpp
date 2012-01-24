#include "TextureContent.h"
#include "main.h"
#include "Texture.h"

BOOST_CLASS_EXPORT_GUID(TextureContent, "TextureContent")

void TextureContent::getFactoryObjectDimensions(int &width, int &height)
{
    g_mainWindow->getGLWindow()->getTextureFactory().getObject(getURI())->getDimensions(width, height);
}

void TextureContent::renderFactoryObject(float tX, float tY, float tW, float tH)
{
    g_mainWindow->getGLWindow()->getTextureFactory().getObject(getURI())->render(tX, tY, tW, tH);
}
