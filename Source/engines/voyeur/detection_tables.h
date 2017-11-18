/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

namespace Voyeur {

static const VoyeurGameDescription gameDescriptions[] = {
	
		// Voyeur DOS English
		{
			"voyeur",
			"",
			//AD_ENTRY1s("a1100200.rl2", "b44630677618d034970ca0a13c1c1237", 336361),
			{
				{"a1100200.rl2", 0, "9656673fd7e5b9190522d38e18a3ced1", 346966},			
			},
			Common::EN_ANY,
			Common::kPlatformDOS,
			ADGF_NO_FLAGS,
			GUIO1(GUIO_NONE)
			//GUIO0()
		},	
	
		// Voyeur CDI German
		{
			"voyeur",
			"",
			{
				{"a1100200.rl2", 0, "b44630677618d034970ca0a13c1c1237", 336361},					
			},
			Common::DE_DEU,
			Common::kPlatformCDi,
			ADGF_NO_FLAGS,
			GUIO1(GUIO_NONE)
			//GUIO0()
		},
	AD_TABLE_END_MARKER
};

} // End of namespace Voyeur
