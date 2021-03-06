#include "common.h"

#include <stdio.h>

typedef struct {
	Timecode tc;
	AVec4f v;
} VarPoint;

#define VAR_MAX_POINTS 256

typedef struct {
	VarDesc desc;
	int used;
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
	scene->bars_per_tick = params->args[0].value.i;
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

	//MSG("%d:%d", pt->tc.bar, pt->tc.tick);

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
	{ "p", 0/*unused*/, 2, 5, {PAF_Time, PAF_Float, PAF_Float, PAF_Float, PAF_Float}, varParseAddPoint },
};

typedef struct {
	Tool head;
	AVec3f pos, up, dir;
	AMat3f axes;
	float focus_distance;
	int forward, right, run;
} ToolCamera;

#define VAR_MAX_MIDI_MAP 128

typedef struct {
	char var_name[MAX_VARIABLE_NAME_LENGTH];
	int override;
	int override_toggle_note;
	int channel;
	int component;
	float min, max;
	float value;
} ToolMidiMap;

typedef struct {
	Tool head;
	VolatileResource *map_source;

	int active_map;
	ToolMidiMap map[2];
} ToolMidi;

static struct {
	const char *filename;
	VolatileResource *source;

	VarScene scene[2];
	int active_scene;

	float bar;
	int frame_sequence;

	struct {
		int override;
		AVec4f value;
	} scratch[VAR_MAX_VARS];

	ToolCamera tool_camera;
	ToolMidi tool_midi;

	int vars_to_export;
} g;

static int varParse(const char *source) {
	const int scene_index = (g.active_scene + 1) % 2;
	VarScene *scene = g.scene + scene_index;
	scene->table.vars = 0;

	for (int i = 0; i < (int)COUNTOF(g.scratch); ++i)
		g.scratch[i].override = 0;

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

void varFrame(float bar) {
	g.bar = bar;
	++g.frame_sequence;

	if (g.source->updated)
		varParse(g.source->bytes);
}

static int varFindIndex(const VarDesc *desc) {
	VarScene *scene = g.scene + g.active_scene;
	for (int i = 0; i < scene->table.vars; ++i) {
		VarData *vd = scene->table.var + i;
		if (strcmp(vd->desc.name, desc->name) == 0) {
			// FIXME check type
			//MSG("%s -> %d", desc->name, i);
			return i;
		}
	}

	return -1;
}

static int varFindOrAllocate(const VarDesc *desc) {
	int index = varFindIndex(desc);
	if (index >= 0)
		return index;

	VarScene *const scene = g.scene + g.active_scene;
	if (scene->table.vars == VAR_MAX_VARS) {
		MSG("Too many vars");
		return -1;
	}

	index = scene->table.vars++;
	MSG("%s %s set to index %d", varGetTypeName(desc->type), desc->name, index);
	VarData *const data = scene->table.var + index;
	g.scratch[index].override = 0;
	data->desc = *desc;
	data->points = 0;

	return index;
}

void varSet(const VarDesc *desc, AVec4f value) {
	const int index = varFindOrAllocate(desc);
	if (index < 0)
		return;

	g.scratch[index].override = 1;
	g.scratch[index].value = value;
}

int varGet(const VarDesc *desc, AVec4f *value) {
	*value = aVec4ff(0);

	const int var_index = varFindOrAllocate(desc);
	if (var_index < 0)
		return -1;

	VarScene *scene = g.scene + g.active_scene;
	VarData *data = scene->table.var + var_index;

	if (g.scratch[var_index].override) {
		*value = g.scratch[var_index].value;
		//MSG("%f, %s = %f %f %f %f", g.bar, data->desc.name, value->x, value->y, value->z, value->w);
		return 0;
	}

	float bar;
	AVec4f v;
	if (data->points) {
		const VarPoint *p = data->point;
		v = p->v;
		bar = p->tc.bar + p->tc.tick / scene->bars_per_tick;
	} else
		return 0;

	for (int i = 1; i < data->points; ++i) {
		const VarPoint *p = data->point + i;
		const float p_bar = p->tc.bar + p->tc.tick / scene->bars_per_tick;
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

void varExportBegin() {
	g.vars_to_export = 0;

	VarScene *scene = g.scene + g.active_scene;
	for (int i = 0; i < scene->table.vars; ++i) {
		VarData *var = scene->table.var + i;
		var->used = 0;

		if (var->points > 1)
			++g.vars_to_export;
	}
}

int varExportGetVarOffset(const VarDesc *desc, AVec4f *value) {
	*value = aVec4ff(0);
	if (g.vars_to_export < 0) {
		MSG("Export mode expected");
		return -1;
	}

	if (strcmp(desc->name, "time") == 0)
		return 0;

	int offset = 1;
	VarScene *scene = g.scene + g.active_scene;
	for (int i = 0; i < scene->table.vars; ++i) {
		VarData *var = scene->table.var + i;
		if (strcmp(var->desc.name, desc->name) == 0) {
			if (var->desc.type != desc->type)
				MSG("WRN: var %s has type %s, but %s is asked",
					desc->name, varGetTypeName(var->desc.type), varGetTypeName(desc->type));

			*value = var->point[0].v;

			if (var->points < 2) {
				*value = var->point[0].v;
				return -1;
			}

			var->used = 1;
			return offset;
		}

		if (var->points > 1)
			offset += var->desc.type;
	}

	MSG("WRN: variable %s not found", desc->name);
	return -1;
}

void varExportEnd() {
	if (g.vars_to_export < 0) {
		MSG("Export mode expected");
		return;
	}

#define MAX_VAR_BUFFER_SIZE 4096
	int buffer_metadata[MAX_VAR_BUFFER_SIZE];
	int *bmeta = buffer_metadata;

	int buffer_timedeltas[MAX_VAR_BUFFER_SIZE];
	int *btimes = buffer_timedeltas;

	float buffer_values[MAX_VAR_BUFFER_SIZE];
	float *bvalues = buffer_values;

	const VarScene *scene = g.scene + g.active_scene;
	for (int i = 0; i < scene->table.vars; ++i) {
		const VarData *var = scene->table.var + i;

		if (var->used < 0)
			MSG("WRN: variable %s unused", var->desc.name);

		if (var->points < 2)
			continue;

		*(bmeta++) = var->desc.type;
		*(bmeta++) = var->points;

		for (int j = 0; j < var->points - 1; ++j) {
			const int t0 = var->point[j].tc.bar * scene->bars_per_tick + var->point[j].tc.tick;
			const int t1 = var->point[j+1].tc.bar * scene->bars_per_tick + var->point[j+1].tc.tick;
			const int dt = t1 - t0;
			if (dt > 255 || dt < 0)
				MSG("Var %s, point %d, invalid dt: %d", var->desc.name, j + 1, dt);

			*(btimes++) = dt;
		}

		for (int j = 0; j < var->points; ++j) {
			for (int k = 0; k < (int)var->desc.type; ++k) {
				const float f = (&var->point[j].v.x)[k];
				*(bvalues++) = f;
			}
		}
	}

	FILE *f = fopen("vars.h", "w");
	fprintf(f, "/* AUTOGENERATED BY tool DO NOT MODIFY */\n");
	fprintf(f, "#define VARS_NUM %d\n", g.vars_to_export);
	fprintf(f, "#pragma data_seg(\".vars_data\")\n");

	fprintf(f, "#pragma data_seg(\".vars.metadata\")\nstatic const unsigned int vars_metadata[] = {\n\t");
	for (const int *p = buffer_metadata; p < bmeta; ++p)
		fprintf(f, "%d, ", *p);
	fprintf(f, "};\n");

	fprintf(f, "#pragma data_seg(\".vars.times\")\nstatic const unsigned int vars_timedeltas[] = {\n\t");
	for (const int *p = buffer_timedeltas; p < btimes; ++p)
		fprintf(f, "%d, ", *p);
	fprintf(f, "};\n");

	fprintf(f, "#pragma data_seg(\".vars.values\")\nstatic const float vars_values[] = {\n\t");
	for (const float *p = buffer_values; p < bvalues; ++p)
		fprintf(f, "%f, ", *p);
	fprintf(f, "};\n");

	fclose(f);

	g.vars_to_export = -1;
}

static void toolCameraRecompute(ToolCamera *cam) {
	cam->dir = aVec3fNormalize(cam->dir);
	const struct AVec3f
			z = aVec3fNeg(cam->dir),
			x = aVec3fNormalize(aVec3fCross(cam->up, z)),
			y = aVec3fCross(z, x);
	cam->axes = aMat3fv(x, y, z);
}

static void toolCameraReadVars(ToolCamera *cam) {
	{
		AVec4f v;
		VarDesc vd = { "cam_pos", VarType_Vec3 };
		varGet(&vd, &v);
		cam->pos = aVec3f(v.x, v.y, v.z);
	}
	{
		AVec4f v;
		VarDesc vd = { "cam_at", VarType_Vec3 };
		varGet(&vd, &v);
		cam->dir = aVec3fSub(aVec3f(v.x, v.y, v.z), cam->pos);
	}
	{
		AVec4f v;
		VarDesc vd = { "focus_distance", VarType_Float };
		varGet(&vd, &v);
		cam->focus_distance = v.x;
	}
	cam->up = aVec3f(0, 1, 0);
	toolCameraRecompute(cam);
}

static void toolCameraActivate(Tool *tool) {
	ToolCamera *cam = (ToolCamera*)tool;
	cam->forward = cam->right = cam->run = 0;

	toolCameraReadVars(cam);
	aAppGrabInput(1);
}

static void toolCameraUpdateOverrides(ToolCamera *cam, int result) {
	VarScene *scene = g.scene + g.active_scene;
	for (int i = 0; i < scene->table.vars; ++i) {
		VarData *vd = scene->table.var + i;
		if (strcmp(vd->desc.name, "cam_pos") == 0) {
			g.scratch[i].override = result > 0;
			g.scratch[i].value = aVec4f3(cam->pos, 0.);
		} else if (strcmp(vd->desc.name, "cam_dir") == 0) {
			g.scratch[i].override = result > 0;
			g.scratch[i].value = aVec4f3(cam->dir, 0.);
		} else if (strcmp(vd->desc.name, "cam_up") == 0) {
			g.scratch[i].override = result > 0;
			g.scratch[i].value = aVec4f3(cam->up, 0.);
		} else if (strcmp(vd->desc.name, "focus_distance") == 0) {
			g.scratch[i].override = result > 0;
			g.scratch[i].value.x = cam->focus_distance;
		}
	}
}

static ToolResult toolCameraEvent(Tool *tool, const ToolInputEvent *evt) {
	ToolCamera *cam = (ToolCamera*)tool;

	if (evt->type == Input_Key) {
		const int pressed = evt->e.key.down;
		switch (evt->e.key.code) {
			case AK_W: cam->forward += pressed?1:-1; return ToolResult_Consumed;
			case AK_S: cam->forward += pressed?-1:1; return ToolResult_Consumed;
			case AK_A: cam->right += pressed?-1:1; return ToolResult_Consumed;
			case AK_D: cam->right += pressed?1:-1; return ToolResult_Consumed;
			case AK_Q: toolCameraUpdateOverrides(cam, 0); return ToolResult_Released;
			case AK_LeftShift: cam->run = pressed; return ToolResult_Consumed;
			case AK_Esc: return ToolResult_Released;
			default: return ToolResult_Ignored;
		}
	} else if (evt->type == Input_MidiCtl) {
		cam->focus_distance = evt->e.midi_ctl.value / 127.f * 20.f;
		return ToolResult_Consumed;
	} else if (evt->type == Input_Pointer) {
		int changed = 0;
		if (evt->e.pointer.dx != 0) {
			const struct AMat3f rot = aMat3fRotateAxis(cam->up, evt->e.pointer.dx * -4e-3f);
			cam->dir = aVec3fMulMat(rot, cam->dir);
			changed = 1;
		}
		if (evt->e.pointer.dy != 0) {
			const struct AMat3f rot = aMat3fRotateAxis(cam->axes.X, evt->e.pointer.dy * -4e-3f);
			cam->dir = aVec3fMulMat(rot, cam->dir);
			changed = 1;
		}

		if (changed) {
			toolCameraRecompute(cam);
			return ToolResult_Consumed;
		}
	}

	return ToolResult_Ignored;
}

static void toolCameraDeactivate(Tool *tool) {
	(void)tool;
	aAppGrabInput(0);
}

static void toolCameraUpdate(Tool *tool, float dt) {
	ToolCamera *cam = (ToolCamera*)tool;
	//MSG("cam %d %d", cam->forward, cam->right);
	//MSG("cam %f %f %f %f", cam->pos.x, cam->pos.y, cam->pos.z);
	if (cam->forward  || cam->right) {
		dt *= 10.f;
		const AVec3f v = {
			(float)(cam->right * (1 + 9 * cam->run)), 0,
			(float)(-cam->forward * (1 + 9 * cam->run))
		};
		cam->pos = aVec3fAdd(cam->pos, aVec3fMulf(cam->axes.X, v.x * dt));
		cam->pos = aVec3fAdd(cam->pos, aVec3fMulf(cam->axes.Y, v.y * dt));
		cam->pos = aVec3fAdd(cam->pos, aVec3fMulf(cam->axes.Z, v.z * dt));
	}
	toolCameraUpdateOverrides(cam, 1);
}

void varInit(const char *filename) {
	g.vars_to_export = -1;
	g.filename = strMakeCopy(filename);

	g.source = resourceOpenFile(filename);
	if (!g.source)
		MSG("Cannot open file %s", filename);

	g.tool_camera.head.activate = toolCameraActivate;
	g.tool_camera.head.deactivate = toolCameraDeactivate;
	g.tool_camera.head.processEvent = toolCameraEvent;
	g.tool_camera.head.update = toolCameraUpdate;
}

void varPrintOverrides() {
	const VarTable *t = &g.scene[g.active_scene].table;
	for (int i = 0; i < t->vars; ++i) {
		if (g.scratch[i].override) {
			MSG("override %s: %f %f %f %f", t->var[i].desc.name,
				g.scratch[i].value.x,
				g.scratch[i].value.y,
				g.scratch[i].value.z,
				g.scratch[i].value.w);
		}
	}
}

VarTools var_tools = {
	&g.tool_camera.head,
	&g.tool_midi.head
};
