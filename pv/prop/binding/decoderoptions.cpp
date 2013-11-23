/*
 * This file is part of the PulseView project.
 *
 * Copyright (C) 2013 Joel Holdsworth <joel@airwebreathe.org.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include <libsigrokdecode/libsigrokdecode.h>

#include "decoderoptions.h"

#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/none_t.hpp>

#include <pv/data/decoderstack.h>
#include <pv/data/decode/decoder.h>
#include <pv/prop/int.h>
#include <pv/prop/string.h>

using namespace boost;
using namespace std;

namespace pv {
namespace prop {
namespace binding {

DecoderOptions::DecoderOptions(
	shared_ptr<pv::data::DecoderStack> decoder_stack,
	shared_ptr<data::decode::Decoder> decoder) :
	_decoder_stack(decoder_stack),
	_decoder(decoder)
{
	assert(_decoder);

	const srd_decoder *const dec = _decoder->decoder();
	assert(dec);

	for (GSList *l = dec->options; l; l = l->next)
	{
		const srd_decoder_option *const opt =
			(srd_decoder_option*)l->data;

		const QString name(opt->desc);

		const Property::Getter getter = bind(
			&DecoderOptions::getter, this, opt->id);
		const Property::Setter setter = bind(
			&DecoderOptions::setter, this, opt->id, _1);

		shared_ptr<Property> prop;

		if (g_variant_is_of_type(opt->def, G_VARIANT_TYPE("x")))
			prop = shared_ptr<Property>(
				new Int(name, "", none, getter, setter));
		else if (g_variant_is_of_type(opt->def, G_VARIANT_TYPE("s")))
			prop = shared_ptr<Property>(
				new String(name, getter, setter));
		else
			continue;

		_properties.push_back(prop);
	}
}

GVariant* DecoderOptions::getter(const char *id)
{
	assert(_decoder);

	// Get the value from the hash table if it is already present
	GVariant *val = (GVariant*)g_hash_table_lookup(
		(GHashTable*)_decoder->options(), id);

	if (!val)
	{
		assert(_decoder->decoder());

		// Get the default value if not
		for (GSList *l = _decoder->decoder()->options; l; l = l->next)
		{
			const srd_decoder_option *const opt =
				(srd_decoder_option*)l->data;
			if (strcmp(opt->id, id) == 0) {
				val = opt->def;
				break;
			}
		}
	}

	if (val)
		g_variant_ref(val);

	return val;
}

void DecoderOptions::setter(const char *id, GVariant *value)
{
	assert(_decoder);
	_decoder->set_option(id, value);

	assert(_decoder_stack);
	_decoder_stack->begin_decode();
}

} // binding
} // prop
} // pv