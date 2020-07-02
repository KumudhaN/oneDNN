/*******************************************************************************
* Copyright 2019-2020 Intel Corporation
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
*******************************************************************************/

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dnnl.h"

#include "dnnl_common.hpp"
#include "dnnl_debug.hpp"
#include "resampling/resampling.hpp"

namespace resampling {

alg_t str2alg(const char *str) {
#define CASE(_alg) \
    if (!strcasecmp(STRINGIFY(_alg), str)) return _alg
    CASE(nearest);
    CASE(linear);
#undef CASE
    assert(!"unknown algorithm");
    return nearest;
}

const char *alg2str(alg_t alg) {
    if (alg == nearest) return "nearest";
    if (alg == linear) return "linear";
    assert(!"unknown algorithm");
    return "unknown algorithm";
}

dnnl_alg_kind_t alg2alg_kind(alg_t alg) {
    if (alg == nearest) return dnnl_resampling_nearest;
    if (alg == linear) return dnnl_resampling_linear;
    assert(!"unknown algorithm");
    return dnnl_alg_kind_undef;
}

int str2desc(desc_t *desc, const char *str) {
    /* canonical form:
     * mbXicXidXihXiwXodXohXowXnS
     *
     * where: Y = {fd, fi, bd}, X is number, S - string
     * note: symbol `_` is ignored
     *
     * implicit rules:
     *  - default values:
     *      mb = 2, ih = oh = id = od = 1, S="wip"
     */

    desc_t d {0};
    d.mb = 2;

    const char *s = str;
    assert(s);

#define CASE_NN(p, c) \
    do { \
        if (!strncmp(p, s, strlen(p))) { \
            ok = 1; \
            s += strlen(p); \
            char *end_s; \
            d.c = strtol(s, &end_s, 10); \
            s += (end_s - s); \
            if (d.c < 0) return FAIL; \
            /* printf("@@@debug: %s: %ld\n", p, d. c); */ \
        } \
    } while (0)
#define CASE_N(c) CASE_NN(#c, c)
    while (*s) {
        int ok = 0;
        CASE_N(mb);
        CASE_N(ic);
        CASE_N(id);
        CASE_N(ih);
        CASE_N(iw);
        CASE_N(od);
        CASE_N(oh);
        CASE_N(ow);
        if (*s == 'n') {
            d.name = s + 1;
            break;
        }
        if (*s == '_') ++s;
        if (!ok) return FAIL;
    }
#undef CASE_NN
#undef CASE_N

    if (d.ic == 0) return FAIL;
    if ((d.id && !d.od) || (!d.id && d.od)) return FAIL;
    if ((d.ih && !d.oh) || (!d.ih && d.oh)) return FAIL;
    if ((d.iw && !d.ow) || (!d.iw && d.ow)) return FAIL;

    if (sanitize_desc(
                d.ndims, {d.od, d.id}, {d.oh, d.ih}, {d.ow, d.iw}, {1, 1}, true)
            != OK)
        return FAIL;

    *desc = d;

    return OK;
}

std::ostream &operator<<(std::ostream &s, const desc_t &d) {
    bool print_d = true, print_h = true, print_w = true;
    print_dhw(print_d, print_h, print_w, d.ndims, {d.od, d.id}, {d.oh, d.ih},
            {d.ow, d.iw});

    if (canonical || d.mb != 2) s << "mb" << d.mb;

    s << "ic" << d.ic;

    if (print_d) s << "id" << d.id;
    if (print_h) s << "ih" << d.ih;
    if (print_w) s << "iw" << d.iw;

    if (print_d) s << "od" << d.od;
    if (print_h) s << "oh" << d.oh;
    if (print_w) s << "ow" << d.ow;

    if (d.name) s << "n" << d.name;

    return s;
}

std::ostream &operator<<(std::ostream &s, const prb_t &p) {
    dump_global_params(s);
    settings_t def;

    if (canonical || p.dir != def.dir[0]) s << "--dir=" << p.dir << " ";
    if (canonical || p.dt != def.dt[0]) s << "--dt=" << p.dt << " ";
    if (canonical || p.tag != def.tag[0]) s << "--tag=" << p.tag << " ";
    if (canonical || p.alg != def.alg[0])
        s << "--alg=" << alg2str(p.alg) << " ";

    s << static_cast<const desc_t &>(p);

    return s;
}

bool prb_t::maybe_skip_nvidia() const {
    if (alg != linear) { return true; }

    if (!(dt == dnnl_f32 || dt == dnnl_f16)) { return true; }

    if (!(ndims == 3 || ndims == 4)) { return true; }

    auto symbolic_tag = convert_tag(tag, ndims);
    if (!(symbolic_tag == dnnl_abc || symbolic_tag == dnnl_abcd
                || symbolic_tag == dnnl_acb || symbolic_tag == dnnl_acdb)) {
        return true;
    }

    return false;
}

} // namespace resampling
