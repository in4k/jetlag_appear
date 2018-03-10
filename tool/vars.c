#include "common.h"

typedef struct {
	Timecode tc;
	AVec4f v;
} VarPoint;

#define VAR_MAX_POINTS 256

typedef struct {
	VarDesc desc;
	int last_used_sequence;
	int points;
	VarPoint point[VAR_MAX_POINTS];
} VarData;

#define VAR_MAX_VARS 64

typedef struct {
	int vars;
	VarData var[VAR_MAX_VARS];
} VarTable;

VarType varGetType(StringView sv) {
	if (strncmp("float", sv.str, sv.length) == 0) {
		return VarType_Float;
	} else if (strncmp("vec2", sv.str, sv.length) == 0) {
		return VarType_Vec2;
	} else if (strncmp("vec3", sv.str, sv.length) == 0) {
		return VarType_Vec3;
	} else if (strncmp("vec4", sv.str, sv.length) == 0) {
		return VarType_Vec4;
	} else
		return VarType_None;
}

const char *varGetTypeName(VarType type) {
	switch (type) {
		case VarType_None: return "none";
		case VarType_Float: return "float";
		case VarType_Vec2: return "vec2";
		case VarType_Vec3: return "vec3";
		case VarType_Vec4: return "vec4";
		default: return "(invalid)";
	}
}

typedef struct {
	float bars_per_tick;
	VarTable table;
} VarScene;

static int varParseTicksPerBar(const ParserCallbackParams *params) {
	VarScene *scene = params->userdata;
	scene->bars_per_tick = 1.f / params->args[0].value.i;
	return 0;
}

static int varParseAddVar(const ParserCallbackParams *params) {
	VarScene *scene = params->userdata;

	const char *name = params->args[1].s;
	const int name_len = params->args[1].slen;
	for (int i = 0; i < scene->table.vars; ++i)
		if (strcmp(scene->table.var[i].desc.name, name) == 0) {
			MSG("Var name %s already exists", name);
			return -1;
		}

	if (scene->table.vars == VAR_MAX_VARS) {
		MSG("Too many vars, limit %d", VAR_MAX_VARS);
		return -1;
	}

	if (name_len >= MAX_VARIABLE_NAME_LENGTH) {
		MSG("Variable name %s is too long (%d), limit %d", name, name_len, MAX_VARIABLE_NAME_LENGTH);
		return -1;
	}

	const VarType type = varGetType(stringView(params->args[0].s, params->args[0].slen));
	if (type == VarType_None) {
		MSG("Variable %s has invalid type %s", name, params->args[0].s);
		return -1;
	}

	VarData *data = scene->table.var + scene->table.vars;
	memcpy(data->desc.name, name, name_len + 1);
	data->desc.type = type;
	data->points = 0;

	++scene->table.vars;
	return 0;
}

static int varParseAddPoint(const ParserCallbackParams *params) {
	VarScene *scene = params->userdata;

	if (scene->table.vars < 1) {
		MSG("No vars defined");
		return -1;
	}

	VarData *data = scene->table.var + scene->table.vars - 1;
	
	if (data->points == VAR_MAX_POINTS) {
		MSG("Variable %s has too many points, limit %d", data->desc.name, VAR_MAX_POINTS);
		return -1;
	}

	VarPoint *pt = data->point + data->points;
	pt->tc = params->args[0].value.time;

	if ((int)data->desc.type != params->num_args - 1)
		MSG("Warning: for var %s type %s point at %d:%d has invalid number of args %d",
				data->desc.name, varGetTypeName(data->desc.type), pt->tc.bar, pt->tc.tick, params->num_args - 1);

	pt->v = aVec4ff(0);
	pt->v.x = params->args[1].value.f;
	if (params->num_args > 2) {
		pt->v.y = params->args[2].value.f;
		if (params->num_args > 3) {
			pt->v.z = params->args[3].value.f;
			if (params->num_args > 4) {
				pt->v.w = params->args[4].value.f;
			}
		}
	}

	++data->points;
	return 0;
}

static const ParserLine vars_line_table[] = {
	{ "ticks_per_bar", 0/*unused*/, 1, 1, {PAF_Int}, varParseTicksPerBar },
	{ "var", 0/*unused*/, 2, 2, {PAF_String, PAF_String}, varParseAddVar },
	{ "p", 0/*unused*/, 2, 4, {PAF_Time, PAF_Float, PAF_Float, PAF_Float}, varParseAddPoint },
};

static struct {
	const char *filename;
	VolatileResource *source;

	VarScene scene[2];
	int active_scene;

	float bar;
	int frame_sequence;
} g;

static int varParse(const char *source) {
	const int scene_index = (g.active_scene + 1) % 2;
	VarScene *scene = g.scene + scene_index;
	scene->table.vars = 0;

	ParserTokenizer parser;
	parser.ctx.line = source;
	parser.ctx.prev_line = 0;
	parser.ctx.line_number = 0;
	parser.line_table = vars_line_table;
	parser.line_table_length = COUNTOF(vars_line_table);
	parser.userdata = scene;

	for (;;) {
		const int result = parserTokenizeLine(&parser);
		if (result == Tokenize_End) {
			g.active_scene = scene_index;
			return 1;
		}

		if (result != Tokenize_Parsed) {
			MSG("Parsing failed at line %d", parser.ctx.line_number);
			return 0;
		}
	}
}

void varInit(const char *filename) {
	g.filename = strMakeCopy(filename);

	g.source = resourceOpenFile(filename);
	if (!g.source)
		MSG("Cannot open file %s", filename);
}

void varFrame(float bar) {
	g.bar = bar;
	++g.frame_sequence;

	if (g.source->updated)
		varParse(g.source->bytes);
}

int varGet(const VarDesc *desc, AVec4f *value) {
	*value = aVec4ff(0);

	VarScene *scene = g.scene + g.active_scene;

	VarData *data = NULL;
	for (int i = 0; i < scene->table.vars; ++i) {
		VarData *vd = scene->table.var + i;
		if (strcmp(vd->desc.name, desc->name) == 0) {
			// FIXME check type
			data = vd;
			break;
		}
	}

	if (!data) {
		if (scene->table.vars == VAR_MAX_VARS)
			return -1;

		data = scene->table.var + scene->table.vars;
		++scene->table.vars;
		data->desc = *desc;
		data->points = 0;
	}

	float bar;
	AVec4f v;
	if (data->points) {
		const VarPoint *p = data->point;
		v = p->v;
		bar = p->tc.bar + p->tc.tick * scene->bars_per_tick;
	} else
		return 0;

	for (int i = 1; i < data->points; ++i) {
		const VarPoint *p = data->point + i;
		const float p_bar = p->tc.bar + p->tc.tick * scene->bars_per_tick;
		if (p_bar >= g.bar) {
			const float range = p_bar - bar;
			const float t = fabs(range) > 1e-6 ? 1.f - (p_bar - g.bar) / range : 1.f;
			v = aVec4fLerp(v, p->v, t);
			break;
		} else {
			v = p->v;
			bar = p_bar;
		}
	}

	*value = v;
	//MSG("%f, %s = %f %f %f %f", g.bar, data->desc.name, v.x, v.y, v.z, v.w);
	return 0;
}
