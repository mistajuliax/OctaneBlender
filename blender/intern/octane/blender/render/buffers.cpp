/*
 * Copyright 2011, Blender Foundation.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <stdlib.h>

#include "buffers.h"
#include "server.h"

#include <OpenImageIO/imageio.h>

#include "util_opengl.h"
#include "util_types.h"

OCT_NAMESPACE_BEGIN

using namespace OIIO_NAMESPACE;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CONSTRUCTOR
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BufferParams::BufferParams() {
	offset_x    = 0;
	offset_y    = 0;
	full_width  = 0;
	full_height = 0;
} //BufferParams()

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Check if buffer sizes and passes are nod equal
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool BufferParams::modified(const BufferParams& params) {
	return !(full_width == params.full_width
        && full_height == params.full_height
        && offset_x == params.offset_x
		&& offset_y == params.offset_y);
} //modified()


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DISPLAY BUFFER
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CONSTRUCTOR
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DisplayBuffer::DisplayBuffer(RenderServer *server_) {
    rgba        = 0;
	server      = server_;
	transparent = true; //TODO: determine from background
} //DisplayBuffer(RenderServer *server_)

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DESTRUCTOR
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DisplayBuffer::~DisplayBuffer() {
	free_buffers();
} //~DisplayBuffer()

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Free the Image-buffer
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void DisplayBuffer::free_buffers() {
    if(rgba) {
        delete[] rgba;
        rgba = 0;
    }
} //free()

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Reset Info and reallocate Image-buffer
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void DisplayBuffer::reset(BufferParams& params_) {
	params = params_;

    free_buffers();
    if(params.full_width > 0 && params.full_height > 0)
        rgba = new uchar4[params.full_width * params.full_height];
} //reset()

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Do the OpenGL draw of the Image-buffer chunk
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool DisplayBuffer::draw(RenderServer *server) {
	if(params.full_width > 0 && params.full_height > 0) {
        if(!server->get_8bit_pixels(rgba, 0, params.full_width, params.full_height)) return false;

		glPushMatrix();
        glTranslatef(max(0, params.offset_x), max(0, params.offset_y), 0.0f);
        glTranslatef(0, 0, 0.0f);

        if(transparent) {
		    glEnable(GL_BLEND);
		    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	    }

	    glPixelZoom(1.0f, 1.0f);
        int iViewport[4];
        glGetIntegerv(GL_VIEWPORT, iViewport);
        glViewport(iViewport[0] + min(0, params.offset_x), iViewport[1] + min(0, params.offset_y), iViewport[2] - min(0, params.offset_x), iViewport[3] - min(0, params.offset_y));
	    glRasterPos2f(0, 0);

	    glDrawPixels(params.full_width, params.full_height, GL_RGBA, GL_UNSIGNED_BYTE, (uint8_t*)rgba);
        glViewport(iViewport[0], iViewport[1], iViewport[2], iViewport[3]);

	    if(transparent) glDisable(GL_BLEND);
		glPopMatrix();
	}
    return true;
} //draw()

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Write the Image to the file
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void DisplayBuffer::write(const string& filename) {
	int w = params.full_width;
	int h = params.full_height;

	if(w == 0 || h == 0) return;

	// Read buffer from server
	server->get_8bit_pixels(rgba, 0, w, h);

	// Write image
	ImageOutput *out = ImageOutput::create(filename);
	ImageSpec spec(w, h, 4, TypeDesc::UINT8);
	int scanlinesize = w * 4 * sizeof(uchar);

	out->open(filename, spec);

	// Conversion for different top/bottom convention
	out->write_image(TypeDesc::UINT8,
		(uchar*)rgba + (h-1)*scanlinesize,
		AutoStride,
		-scanlinesize,
		AutoStride);

	out->close();
	delete out;
} //write()

OCT_NAMESPACE_END

