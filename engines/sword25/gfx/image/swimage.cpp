/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/*
 * This code is based on Broken Sword 2.5 engine
 *
 * Copyright (c) Malte Thiesen, Daniel Queteschiner and Michael Elsdoerfer
 *
 * Licensed under GNU GPL v2
 *
 */

#include "sword25/package/packagemanager.h"
#include "sword25/gfx/image/imgloader.h"
#include "sword25/gfx/image/swimage.h"

namespace Sword25 {

SWImage::SWImage(const Common::String &filename, bool &result) : _image() {
	result = false;

	PackageManager *pPackage = Kernel::getInstance()->getPackage();
	assert(pPackage);

	// Load file
	byte *pFileData;
	uint fileSize;
	pFileData = pPackage->getFile(filename, &fileSize);
	if (!pFileData) {
		error("File \"%s\" could not be loaded.", filename.c_str());
		return;
	}

	// Uncompress the image
	if (!ImgLoader::decodePNGImage(pFileData, fileSize, &_image)) {
		error("Could not decode image.");
		return;
	}

	// Cleanup FileData
	delete[] pFileData;

	result = true;
	return;
}

SWImage::~SWImage() {
	_image.free();
}


bool SWImage::blit(int posX, int posY,
					  int flipping,
					  Common::Rect *pPartRect,
					  uint color,
					  int width, int height,
					  RectangleList *updateRects) {
	error("Blit() is not supported.");
	return false;
}

bool SWImage::fill(const Common::Rect *pFillRect, uint color) {
	error("Fill() is not supported.");
	return false;
}

bool SWImage::setContent(const byte *pixeldata, uint size, uint offset, uint stride) {
	error("SetContent() is not supported.");
	return false;
}

uint SWImage::getPixel(int x, int y) {
	assert(x >= 0 && x < _image.w);
	assert(y >= 0 && y < _image.h);

	byte a, r, g, b;
	_image.format.colorToARGB(_image.getPixel(x, y), a, r, g, b);

	return BS_ARGB(a, r, g, b);
}

} // End of namespace Sword25
