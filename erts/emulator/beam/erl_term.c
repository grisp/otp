/*
 * %CopyrightBegin%
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright Ericsson AB 2000-2025. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * %CopyrightEnd%
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include "sys.h"
#include "erl_vm.h"
#include "global.h"
#include "erl_map.h"
#include "erl_bits.h"
#include "erl_binary.h"
#include <stdlib.h>
#include <stdio.h>

void
erts_set_literal_tag(Eterm *term, Eterm *hp_start, Eterm hsz)
{
#ifdef TAG_LITERAL_PTR
    Eterm *hp_end, *hp;
    
    hp_end = hp_start + hsz;
    hp = hp_start;

    while (hp < hp_end) {
	switch (primary_tag(*hp)) {
	case TAG_PRIMARY_BOXED:
	case TAG_PRIMARY_LIST:
	    *hp |= TAG_LITERAL_PTR;
	    break;
	case TAG_PRIMARY_HEADER:
	    if (*hp == HEADER_SUB_BITS) {
                /* Tag the `orig` field as a literal. It's the last field
                 * inside the thing structure so we can handle it by pretending
                 * it's not part of the thing. */
                hp += thing_arityval(*hp) - 1;
            } else if (header_is_thing(*hp)) {
                hp += thing_arityval(*hp);
            }
	    break;
	default:
	    break;
	}
	
	hp++;
    }
    if (is_boxed(*term) || is_list(*term))
	*term |= TAG_LITERAL_PTR;
#endif
}

void
erts_term_init(void)
{
#ifdef ERTS_ORDINARY_REF_MARKER
    /* Ordinary and magic references of same size... */

    ErtsRefThing ref_thing;

    ERTS_CT_ASSERT(ERTS_ORDINARY_REF_MARKER == ~((Uint32)0));
    ref_thing.m.header = ERTS_REF_THING_HEADER;
    ref_thing.m.mb = (ErtsMagicBinary *) ~((UWord) 3);
    ref_thing.m.next = (struct erl_off_heap_header *) ~((UWord) 3);
    if (ref_thing.o.marker == ERTS_ORDINARY_REF_MARKER)
        ERTS_INTERNAL_ERROR("Cannot differentiate between magic and ordinary references");

    ERTS_CT_ASSERT(offsetof(ErtsORefThing,marker) != 0);
    ERTS_CT_ASSERT(sizeof(ErtsORefThing) == sizeof(ErtsMRefThing));
#  ifdef ERTS_MAGIC_REF_THING_HEADER
#    error Magic ref thing header should not have been defined...
#  endif

#else
    /* Ordinary and magic references of different sizes... */

#  ifndef ERTS_MAGIC_REF_THING_HEADER
#    error Magic ref thing header should have been defined...
#  endif
    ERTS_CT_ASSERT(sizeof(ErtsORefThing) != sizeof(ErtsMRefThing));

#endif

    ERTS_CT_ASSERT(ERTS_REF_THING_SIZE*sizeof(Eterm) == sizeof(ErtsORefThing));
    ERTS_CT_ASSERT(ERTS_MAGIC_REF_THING_SIZE*sizeof(Eterm) == sizeof(ErtsMRefThing));

    ERTS_CT_ASSERT((POS_BIG_SUBTAG & _BIG_TAG_MASK)
                    == POS_BIG_SUBTAG);
    ERTS_CT_ASSERT((NEG_BIG_SUBTAG & _BIG_TAG_MASK)
                    == POS_BIG_SUBTAG);
    ERTS_CT_ASSERT((HEAP_BITS_SUBTAG & _BITSTRING_TAG_MASK)
                    == HEAP_BITS_SUBTAG);
    ERTS_CT_ASSERT((SUB_BITS_SUBTAG & _BITSTRING_TAG_MASK)
                    == HEAP_BITS_SUBTAG);
    ERTS_CT_ASSERT((_TAG_HEADER_EXTERNAL_PID & _EXTERNAL_TAG_MASK)
                    == _TAG_HEADER_EXTERNAL_PID);
    ERTS_CT_ASSERT((_TAG_HEADER_EXTERNAL_PORT & _EXTERNAL_TAG_MASK)
                    == _TAG_HEADER_EXTERNAL_PID);
    ERTS_CT_ASSERT((_TAG_HEADER_EXTERNAL_REF & _EXTERNAL_TAG_MASK)
                    == _TAG_HEADER_EXTERNAL_PID);

#ifdef DEBUG
    {
        /* Check that the tag masks cannot confuse tags outside of their
         * category. */
        const Eterm tags[] = {ARITYVAL_SUBTAG,
                              POS_BIG_SUBTAG,
                              NEG_BIG_SUBTAG,
                              REF_SUBTAG,
                              FUN_SUBTAG,
                              FLOAT_SUBTAG,
                              HEAP_BITS_SUBTAG,
                              SUB_BITS_SUBTAG,
                              BIN_REF_SUBTAG,
                              MAP_SUBTAG,
                              EXTERNAL_PID_SUBTAG,
                              EXTERNAL_PORT_SUBTAG,
                              EXTERNAL_REF_SUBTAG};

        for (int i = 0; i < (sizeof(tags) / sizeof(tags[0])); i++) {
            const Eterm tag = tags[i];

            if ((tag & _EXTERNAL_TAG_MASK) == _TAG_HEADER_EXTERNAL_PID) {
                ASSERT((tag == EXTERNAL_PID_SUBTAG) ||
                       (tag == EXTERNAL_PORT_SUBTAG) ||
                       (tag == EXTERNAL_REF_SUBTAG));
            }

            if ((tag & _BITSTRING_TAG_MASK) == HEAP_BITS_SUBTAG) {
                ASSERT((tag == HEAP_BITS_SUBTAG) ||
                       (tag == SUB_BITS_SUBTAG));
            }

            if ((tag & _BIG_TAG_MASK) == POS_BIG_SUBTAG) {
                ASSERT((tag == POS_BIG_SUBTAG) ||
                       (tag == NEG_BIG_SUBTAG));
            }
        }
    }
#endif
}

/*
 * XXX: define NUMBER_CODE() here when new representation is used
 */

#if ET_DEBUG
#define ET_DEFINE_CHECKED(FUNTY,FUN,ARGTY,PRECOND) \
FUNTY checked_##FUN(ARGTY x, const char *file, unsigned line) \
{ \
    ET_ASSERT(PRECOND(x),file,line); \
    return _unchecked_##FUN(x); \
}

ET_DEFINE_CHECKED(Eterm,make_boxed,const Eterm*,_is_taggable_pointer);
ET_DEFINE_CHECKED(int,is_boxed,Eterm,!is_header);
ET_DEFINE_CHECKED(Eterm*,boxed_val,Eterm,_boxed_precond);
ET_DEFINE_CHECKED(Eterm,make_list,const Eterm*,_is_taggable_pointer);
ET_DEFINE_CHECKED(int,is_not_list,Eterm,!is_header);
ET_DEFINE_CHECKED(Eterm*,list_val,Eterm,_list_precond);
ET_DEFINE_CHECKED(Uint,unsigned_val,Eterm,is_small);
ET_DEFINE_CHECKED(Sint,signed_val,Eterm,is_small);
ET_DEFINE_CHECKED(Uint,atom_val,Eterm,is_atom);
ET_DEFINE_CHECKED(Uint,header_arity,Eterm,is_header);
ET_DEFINE_CHECKED(Uint,arityval,Eterm,is_sane_arity_value);
ET_DEFINE_CHECKED(Uint,thing_arityval,Eterm,is_thing);
ET_DEFINE_CHECKED(Uint,thing_subtag,Eterm,is_thing);
ET_DEFINE_CHECKED(Eterm*,bitstring_val,Eterm,is_bitstring);
ET_DEFINE_CHECKED(Eterm*,fun_val,Eterm,is_any_fun);
ET_DEFINE_CHECKED(int,bignum_header_is_neg,Eterm,_is_bignum_header);
ET_DEFINE_CHECKED(Eterm,bignum_header_neg,Eterm,_is_bignum_header);
ET_DEFINE_CHECKED(Uint,bignum_header_arity,Eterm,_is_bignum_header);
ET_DEFINE_CHECKED(Eterm*,big_val,Eterm,is_big);
ET_DEFINE_CHECKED(Eterm*,float_val,Eterm,is_float);
ET_DEFINE_CHECKED(Eterm*,tuple_val,Eterm,is_tuple);
ET_DEFINE_CHECKED(struct erl_node_*,internal_pid_node,Eterm,is_internal_pid);
ET_DEFINE_CHECKED(struct erl_node_*,internal_port_node,Eterm,is_internal_port);
ET_DEFINE_CHECKED(Eterm*,internal_ref_val,Eterm,is_internal_ref);
ET_DEFINE_CHECKED(Uint32*,internal_magic_ref_numbers,Eterm,is_internal_magic_ref);
ET_DEFINE_CHECKED(Uint32*,internal_non_magic_ref_numbers,Eterm,is_internal_non_magic_ref);
ET_DEFINE_CHECKED(struct erl_node_*,internal_ref_node,Eterm,is_internal_ref);
ET_DEFINE_CHECKED(Eterm*,external_val,Eterm,is_external);
ET_DEFINE_CHECKED(Uint,external_data_words,Eterm,is_external);
ET_DEFINE_CHECKED(Uint,external_pid_data_words,Eterm,is_external_pid);
ET_DEFINE_CHECKED(struct erl_node_*,external_pid_node,Eterm,is_external_pid);
ET_DEFINE_CHECKED(Uint,external_port_data_words,Eterm,is_external_port);
ET_DEFINE_CHECKED(Uint*,external_port_data,Eterm,is_external_port);
ET_DEFINE_CHECKED(struct erl_node_*,external_port_node,Eterm,is_external_port);
ET_DEFINE_CHECKED(Uint,external_ref_data_words,Eterm,is_external_ref);
ET_DEFINE_CHECKED(Uint32*,external_ref_data,Eterm,is_external_ref);
ET_DEFINE_CHECKED(struct erl_node_*,external_ref_node,Eterm,is_external_ref);
ET_DEFINE_CHECKED(Uint,external_thing_data_words,const ExternalThing*,is_thing_ptr);

ET_DEFINE_CHECKED(Eterm,make_cp,ErtsCodePtr,_is_legal_cp);
ET_DEFINE_CHECKED(ErtsCodePtr,cp_val,Eterm,is_CP);
ET_DEFINE_CHECKED(Uint,catch_val,Eterm,is_catch);
ET_DEFINE_CHECKED(Uint,loader_x_reg_index,Uint,_is_loader_x_reg);
ET_DEFINE_CHECKED(Uint,loader_y_reg_index,Uint,_is_loader_y_reg);

#endif	/* ET_DEBUG */
