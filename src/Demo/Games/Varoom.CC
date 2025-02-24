




		$SP,"<1>",BI=1$



		$SP,"<2>",BI=2$







		$SP,"<3>",BI=3$




















		$SP,"<4>",BI=4$









								 $SP,"<5>",BI=5$



























								 $SP,"<6>",BI=6$












RegDefault("ZenithOS/Varoom", "F64 best_score=9999;\n");
RegExe("ZenithOS/Varoom");

F64  distance, t0, tf;
Bool game_over;

#define BORDER		7500
#define RADIUS		10000
#define WIDTH 		2000
#define SHOULDER	200
#define D_� 		(2 * � / 360) 	//Curve track slice is one degree.
#define D_S 		(2 * � * RADIUS / 360) //Straight track is degree at 10000.
#define DIPS		5
#define DIP_DEPTH	50
class Track
{
	Track			*next, *last;
	I32				 num;
	CColorROPU16	 c, pad;
	I64				 x, z;
	F64				 �, d;
	CD3I32			 left[4], center[4], right[4];

} track_head, *track_start[MP_PROCESSORS_NUM], *track_end[MP_PROCESSORS_NUM];

CDC *track_map;
#define MAP_BITS	9
I64  t_minx, t_maxx, t_minz, t_maxz;

#define BUSHES_NUM	512
class Bush
{
	CD3I32	 p;
	Bool	 sym, pad[3];
	U8		*img;

} b[BUSHES_NUM];

#define CARS_NUM	8
class Car
{
	CD3I32	 p;
	F64		 �, d�, speed;
	U8		*img;
	Track	*t;

} c[CARS_NUM];

I64 DipY(I64 x, I64 z)
{
	F64 m, a;

	R2P(&m, &a, x, z);

	return DIP_DEPTH * m * Cos(DIPS * a) / RADIUS;
}

#define CAR_LENGTH	400

F64 Dip�(I64 x, I64 z, F64 �)
{
	F64 y_front, y_back;

	y_front = DipY(x - CAR_LENGTH / 2 * Cos(�), z - CAR_LENGTH / 2 * Sin(�));
	y_back  = DipY(x + CAR_LENGTH / 2 * Cos(�), z + CAR_LENGTH / 2 * Sin(�));

	return ASin((y_front - y_back) / CAR_LENGTH);
}

Track *TrackFind(Track *_tmpt, I64 x, I64 z)
{
	Track	*res = _tmpt, *tmpt;
	I64		 dd, best = SqrI64(res->x - x) + SqrI64(res->z - z);

	tmpt = _tmpt;
	while (TRUE)
	{
		tmpt = tmpt->next;
		if (tmpt == &track_head)
			tmpt = tmpt->next;
		dd = SqrI64(tmpt->x - x) + SqrI64(tmpt->z - z);
		if (dd < best)
		{
			best = dd;
			res = tmpt;
		}
		else
			break;
	}
	tmpt = _tmpt;
	while (TRUE)
	{
		tmpt = tmpt->last;
		if (tmpt == &track_head)
			tmpt = tmpt->last;
		dd = SqrI64(tmpt->x - x) + SqrI64(tmpt->z - z);
		if (dd < best)
		{
			best = dd;
			res = tmpt;
		}
		else
			break;
	}

	return res;
}

U0 TrackSlice(F64 *_x, F64 *_z, F64 �, F64 d)
{
	F64		 x = *_x, z = *_z, c = Cos(�), s = Sin(�), dx = d * s, dz = -d * c;
	Track	*tmpt, *last = track_head.last;

	if (last == &track_head)
		last = NULL;

	tmpt =CAlloc(sizeof(Track));
	if (last)
	{
		MemCopy(&tmpt->center[0], &last->center[3], sizeof(CD3I32));
		MemCopy(&tmpt->center[1], &last->center[2], sizeof(CD3I32));
	}
	tmpt->center[2].x = x + (WIDTH / 2) * c + dx;
	tmpt->center[2].z = z + (WIDTH / 2) * s + dz;
	tmpt->center[2].y = DipY(tmpt->center[2].x, tmpt->center[2].z);
	tmpt->center[3].x = x - (WIDTH / 2) * c + dx;
	tmpt->center[3].z = z - (WIDTH / 2) * s + dz;
	tmpt->center[3].y = DipY(tmpt->center[3].x, tmpt->center[3].z);

	if (last)
	{
		MemCopy(&tmpt->left[0], &last->left[3], sizeof(CD3I32));
		MemCopy(&tmpt->left[1], &last->left[2], sizeof(CD3I32));
	}
	tmpt->left[2].x = x - (WIDTH / 2) * c + dx;
	tmpt->left[2].z = z - (WIDTH / 2) * s + dz;
	tmpt->left[2].y = DipY(tmpt->left[2].x, tmpt->left[2].z);
	tmpt->left[3].x = x - (WIDTH / 2 + SHOULDER) * c + dx;
	tmpt->left[3].z = z - (WIDTH / 2 + SHOULDER) * s + dz;
	tmpt->left[3].y = DipY(tmpt->left[3].x, tmpt->left[3].z);

	if (last)
	{
		MemCopy(&tmpt->right[0], &last->right[3], sizeof(CD3I32));
		MemCopy(&tmpt->right[1], &last->right[2], sizeof(CD3I32));
	}
	tmpt->right[2].x = x + (WIDTH / 2 + SHOULDER) * c + dx;
	tmpt->right[2].z = z + (WIDTH / 2 + SHOULDER) * s + dz;
	tmpt->right[2].y = DipY(tmpt->right[2].x, tmpt->right[2].z);
	tmpt->right[3].x = x + (WIDTH / 2) * c + dx;
	tmpt->right[3].z = z + (WIDTH / 2) * s + dz;
	tmpt->right[3].y = DipY(tmpt->right[3].x, tmpt->right[3].z);

	tmpt->x		= x;
	tmpt->z		= z;
	tmpt->�		= �;
	tmpt->num	= track_head.last->num + 1;
	tmpt->d		= track_head.last->d + d;

	QueueInsert(tmpt, track_head.last);

	if (tmpt->num & 1)
		tmpt->c = RED;
	else
		tmpt->c = WHITE;

	if (x < t_minx)
		t_minx = x;

	if (x > t_maxx)
		t_maxx = x;

	if (z < t_minz)
		t_minz = z;

	if (z > t_maxz)
		t_maxz = z;

	x += dx;
	*_x = x;
	z += dz;
	*_z = z;

	if (x < t_minx)
		t_minx = x;

	if (x > t_maxx)
		t_maxx = x;

	if (z < t_minz)
		t_minz = z;

	if (z > t_maxz)
		t_maxz = z;
}

U0 CoupleEnds()
{
	Track *first = track_head.next, *last = track_head.last;

	MemCopy(&first->center[0], &last->center[3], sizeof(CD3I32));
	MemCopy(&first->center[1], &last->center[2], sizeof(CD3I32));
	MemCopy(&first->left[0],   &last->left[3],	 sizeof(CD3I32));
	MemCopy(&first->left[1],   &last->left[2],	 sizeof(CD3I32));
	MemCopy(&first->right[0],  &last->right[3],  sizeof(CD3I32));
	MemCopy(&first->right[1],  &last->right[2],  sizeof(CD3I32));
}

U0 InitTrack()
{
	I64		 i, j;
	Track	*tmpt;
	F64		 x, z, �, d;

	MemSet(&track_head, 0, sizeof(Track));
	QueueInit(&track_head);

	t_minx = t_minz = I64_MAX;
	t_maxx = t_maxz = I64_MIN;

	x = 0;
	z = 0;
	� = 0;
	for (d = 0; d < 6 * RADIUS; d += D_S)
		TrackSlice(&x, &z, �, D_S);

	for (i = 0; i < 180; i++, � += D_�)
		TrackSlice(&x, &z, �, D_� * RADIUS);

	for (d = 0; d < RADIUS; d += D_S)
		TrackSlice(&x, &z, �, D_S);

	for (i = 0; i < 90; i++, � -= D_�)
		TrackSlice(&x, &z, �, D_� * RADIUS);

	for (i = 0; i < 180; i++, � += D_�)
		TrackSlice(&x, &z, �, D_� * RADIUS);

	for (i = 0; i < 90; i++, � -= D_�)
		TrackSlice(&x, &z, �, D_� * RADIUS);

	for (d = 0; d < RADIUS; d += D_S)
		TrackSlice(&x, &z, �, D_S);

	for (i = 0; i < 180; i++, � += D_�)
		TrackSlice(&x, &z, �, D_� * RADIUS);

	CoupleEnds;

	tmpt = track_head.next;
	for (i = 0; i < mp_count; i++)
	{
		j = (i + 1) * track_head.last->num / mp_count + 1;
		track_start[i] = tmpt;
		while (tmpt != &track_head && tmpt->num != j)
			tmpt = tmpt->next;
		track_end[i] = tmpt;
	}

	t_minx -= BORDER;
	t_minz -= BORDER;
	t_maxx += BORDER;
	t_maxz += BORDER;
	track_map = DCNew((t_maxx - t_minx + 1 << MAP_BITS - 1) >> MAP_BITS, (t_maxz - t_minz + 1 << MAP_BITS - 1) >> MAP_BITS);

	track_map->color = LTGRAY;
	GrRect(track_map, 0, 0, track_map->width, track_map->height);
	tmpt=track_head.next;
	track_map->color = YELLOW;
	track_map->thick = 3;
	while (tmpt != &track_head)
	{
		GrPlot3(track_map, track_map->width - (tmpt->x - t_minx) >> MAP_BITS, (tmpt->z - t_minz) >> MAP_BITS, 0);
		tmpt = tmpt->next;
	}
}

#define HORIZON_DIP 		200

Bool PrepPoly(CD3I32 *p, I64 *r, I64 cx, I64 h, CD3I32 *poly)
{
	I64 x, y, z, i;
	F64 s;

	for (i = 0; i < 4; i++)
	{
		x = p[i].x - c[0].p.x;
		y = p[i].y - c[0].p.y;
		z = p[i].z - c[0].p.z;
		Mat4x4MulXYZ(r, &x, &y, &z);
		s = 100.0 / (AbsI64(z) + 50);
		poly[i].y = s * y + h;
		if (z < -200 || !(-h < poly[i].y < 2 * h))
			return FALSE;
		poly[i].x = s * x + cx;
		poly[i].z = z + GR_Z_ALL;
	}

	return TRUE;
}

I64 mp_not_done_flags;

U0 MPUpdateWin(CDC *dc2)
{
	CTask	*task = dc2->win_task;
	I64 	 i, x, y, z, w = task->pix_width, h = task->pix_height, r[16], cx = w >> 1;
	F64 	 s, dip_� = Dip�(c[0].p.x, c[0].p.z, c[0].�);
	Car 	*tmpc;
	CD3I32	 poly[4];
	Track	*tmpt, *tmpt1;
	CDC 	*dc = DCAlias(gr.dc2, task);

	Mat4x4IdentEqu(r);
	Mat4x4RotY(r, � - c[0].�);
	Mat4x4RotX(r, 75 * � / 180 - dip_�);

	dc->depth_buf = dc2->depth_buf;

	//Track
	tmpt  = track_start[Gs->num];
	tmpt1 = track_end  [Gs->num];
	while (tmpt != tmpt1)
	{
		dc->color = DKGRAY;
		if (PrepPoly(&tmpt->center, r, cx, h, poly))
		{
			GrFillPoly3(dc, 4, poly);
			dc->color = tmpt->c;
			if (PrepPoly(&tmpt->left, r, cx, h, poly))
				GrFillPoly3(dc, 4, poly);
			if (PrepPoly(&tmpt->right, r, cx, h, poly))
				GrFillPoly3(dc, 4, poly);
		}
		tmpt = tmpt->next;
	}

	dc->flags |= DCF_TRANSFORMATION;
	for (i = Gs->num; i < BUSHES_NUM; i += mp_count)
	{
		x = b[i].p.x - c[0].p.x;
		y = b[i].p.y - c[0].p.y;
		z = b[i].p.z - c[0].p.z;
		Mat4x4MulXYZ(r, &x, &y, &z);
		if (z > 0)
		{
			s = 100.0 / (AbsI64(z) + 50);
			Mat4x4IdentEqu(dc->r);
			Mat4x4Scale(dc->r, s * 2);
			DCMat4x4Set(dc, dc->r);
			if (b[i].sym)
			{
				dc->flags |= DCF_SYMMETRY | DCF_JUST_MIRROR;
				DCSymmetrySet(dc, s * x + cx, s * y + h, s * x + cx, s * y + h + 10);
			}
			Sprite3B(dc, s * x + cx, s * y + h, z + GR_Z_ALL, b[i].img);
			dc->flags&=~(DCF_SYMMETRY | DCF_JUST_MIRROR);
		}
	}
	for (i = Gs->num + 1; i < CARS_NUM; i += mp_count)
	{
		tmpc = &c[i];
		x = tmpc->p.x - c[0].p.x;
		y = tmpc->p.y - c[0].p.y;
		z = tmpc->p.z - c[0].p.z;
		Mat4x4MulXYZ(r, &x, &y, &z);
		if (z > 0)
		{
			s = 100.0 / (AbsI64(z) + 50);
			Mat4x4IdentEqu(dc->r);
			Mat4x4Scale(dc->r, s * 2);
			Mat4x4RotX(dc->r, Dip�(tmpc->p.x, tmpc->p.z, -tmpc->�));
			Mat4x4RotY(dc->r, tmpc->� - c[0].�);
			DCMat4x4Set(dc, dc->r);
			Sprite3B(dc, s * x + cx, s * y + h, z + GR_Z_ALL, tmpc->img);
		}
	}
	dc->depth_buf = NULL;
	DCDel(dc);
	LBtr(&mp_not_done_flags, Gs->num);
}

U0 VRTransform(CDC *dc, I64 *x, I64 *y, I64 *z)
{
	I64 zz;

	Mat4x4MulXYZ(dc->r, x, y, z);
	zz = 400 + *z;
	if (zz < 1)
		zz = 1;
	*x = 400 * *x / zz;
	*y = 400 * *y / zz;
	*x += dc->x;
	*y += dc->y;
	*z += dc->z;
}

U0 DrawIt(CTask *task, CDC *dc)
{
	I64 	 i, x, y, z, 
			 w = task->pix_width, 
			 h = task->pix_height, r[16], 
			 cx = w >> 1;
	F64 	 s, dip_� = Dip�(c[0].p.x, c[0].p.z, c[0].�);
	Car 	*tmpc = &c[0];

	dc->color = LTCYAN;
	GrRect(dc, 0, 0, w, HORIZON_DIP * Sin(dip_�) + FONT_HEIGHT * 4.5);

	Mat4x4IdentEqu(r);
	Mat4x4RotY(r, � - c[0].�);
	Mat4x4RotX(r, 75 * � / 180 - dip_�);

	DCDepthBufAlloc(dc);

	//Sun
	x = c[0].p.x;
	y = 0;
	z = 1000000 - c[0].p.z;
	Mat4x4MulXYZ(r, &x, &y, &z);
	s = 100.0 / (AbsI64(z) + 50);
	if (y < 0)
	{
		dc->color = BROWN;
		GrCircle(dc, s * x + cx, 15 + HORIZON_DIP * Sin(dip_�), 15);
		dc->color = YELLOW;
		GrFloodFill(dc, s * x + cx, 15 + HORIZON_DIP * Sin(dip_�));
	}

	mp_not_done_flags = 1 << mp_count - 1;
	for (i = 0; i < mp_count; i++)
		JobQueue(&MPUpdateWin, dc, i);
	while (mp_not_done_flags)
		Yield;

	Mat4x4IdentEqu(r);
	Mat4x4RotY(r, tmpc->d�);
	Mat4x4RotX(r, 0.4 - 8 * dip_�); //Made this up
	dc->transform = &VRTransform;
	dc->x = task->pix_width >> 1;
	dc->y = task->pix_height - 150;
	dc->z = GR_Z_ALL;
	Sprite3Mat4x4B(dc, 0, 0, -100, c[0].img, r);

	//Map
	GrBlot(dc, w - track_map->width, h - track_map->height, track_map);

	dc->thick = 2;
	for (i = 0; i < CARS_NUM; i++)
	{
		if (i)
			dc->color = LTPURPLE;
		else
			dc->color = LTCYAN;
		GrPlot3(dc, w - (c[i].p.x - t_minx) >> MAP_BITS, h - track_map->height + (c[i].p.z - t_minz) >> MAP_BITS, 0);
	}

	if (game_over)
	{
		dc->color = LTRED;
		if (tf)
		{
			s = tf - t0;
			if (Blink)
				GrPrint(dc, (w - FONT_WIDTH * 14) / 2, (h - FONT_HEIGHT) / 2, "Game Completed");
		}
		else
		{
			s = 99.9;
			if (Blink)
				GrPrint(dc, (w - FONT_WIDTH * 9) / 2, (h - FONT_HEIGHT) / 2, "Game Over");
		}
	}
	else
		s = tS-t0;
	dc->color = BLACK;
	GrPrint(dc, 0, 0, "%0.1f%% Time:%0.2f Best:%0.2f", 100.0 * distance / track_head.last->d, s, best_score);
}

U0 AnimateTask(I64)
{
	Car 	*tmpc;
	I64		 i, x, z;
	Bool	 on_track;
	Track	*tmpt, *tmpt2;

	while (TRUE)
	{
		if (!game_over)
			Sound(12.0 * Log2(c[0].speed / 500 + 0.7));
		else
			Sound;
		for (i = 0; i < CARS_NUM; i++)
		{
			tmpc = &c[i];
			tmpc->p.x -= 0.01 * tmpc->speed * Cos(tmpc->� - � / 2);
			tmpc->p.z += 0.01 * tmpc->speed * Sin(tmpc->� - � / 2);
			tmpt = TrackFind(tmpc->t, tmpc->p.x, tmpc->p.z);
			if (i)
			{
				if (tmpt != tmpc->t)
				{
					tmpt2 = tmpt->next;
					if (tmpt2 == &track_head)
						tmpt2 = tmpt2->next;
					tmpc->� = Arg(-tmpt2->z + tmpc->p.z, -tmpt2->x + tmpc->p.x);
				}
			}
			else
			{
				tmpc->� += 0.01 * tmpc->d�;
				x =track_map->width - (tmpc->p.x - t_minx) >> MAP_BITS;
				z = (tmpc->p.z - t_minz) >> MAP_BITS;
				if (GrPeek(track_map, x, z) != YELLOW)
				{
					on_track = FALSE;
					tmpc->speed -= 0.01 * tmpc->speed;
					if (tmpc->speed < 0)
						tmpc->speed = 0;
				}
				else
					on_track = TRUE;
			}
			tmpc->t = tmpt;
			tmpc->p.y = DipY(tmpc->p.x, tmpc->p.z);
		}
		if (!game_over && on_track)
		{
			for (i = 1; i < CARS_NUM; i++)
				if (D3I32DistSqr(&c[i].p, &c[0].p) < CAR_LENGTH >> 1 * CAR_LENGTH >> 1)
				{
					game_over = TRUE;
					Noise(500, 22, 34);
					Sleep(500);
					break;
				}
			if (!game_over)
			{
				distance += 0.01 * c[0].speed;
				if (distance > track_head.last->d && c[0].t->num < track_head.last->num >> 1)
				{
					tf = tS;
					game_over = TRUE;
					Beep;
					if (tf - t0 < best_score)
					{
						best_score = tf - t0;
						Beep;
					}
				}
			}
		}
		Sleep(10);
	}
}

U8 *imgs[8]={$IB,"<1>",BI=1$, $IB,"<1>",BI=1$, $IB,"<2>",BI=2$, $IB,"<2>",BI=2$, $IB,"<3>",BI=3$, $IB,"<4>",BI=4$, $IB,"<4>",BI=4$, $IB,"<4>",BI=4$};


U0 InitBushes()
{
	Bush	*tmpb;
	I64		 i, j, x, z;

	track_map->color = LTGREEN;
	track_map->thick = 1;
	for (i = 0; i < BUSHES_NUM; i++)
	{
		tmpb = &b[i];

ib_restart:
		tmpb->p.x = Rand * (t_maxx - t_minx) + t_minx;
		tmpb->p.z = Rand * (t_maxz - t_minz) + t_minz;
		x = track_map->width - (tmpb->p.x - t_minx) >> MAP_BITS;
		z = (tmpb->p.z - t_minz) >> MAP_BITS;
		for (j = 0; j < 8; j++)
			if (GrPeek(track_map, x + gr_x_offsets[j], z + gr_y_offsets[j]) != LTGRAY)
				goto ib_restart;

		GrPlot(track_map, x, z);
		tmpb->p.y = DipY(tmpb->p.x, tmpb->p.z);
		tmpb->sym = RandU16 & 1;
		tmpb->img = imgs[i & 7];
	}
}


U0 Init()
{
	Car 	*tmpc;
	Track	*tmpt;
	F64 	 d;
	I64 	 i;

	InitTrack;
	InitBushes;
	tmpt = track_head.next;
	for (i = 0; i < CARS_NUM; i++)
	{
		tmpc = &c[i];
		tmpc->t		= tmpt;
		tmpc->p.x	= tmpt->x;
		tmpc->p.z	= tmpt->z;
		tmpc->p.y	= DipY(tmpc->p.x, tmpc->p.z);
		tmpc->�		= -tmpt->�;
		tmpc->d�	= 0;
		if (!i)
		{
			tmpc->img	= $IB,"<5>",BI=5$;
			tmpc->speed	= 0;
		}
		else
		{
			tmpc->img	= $IB,"<6>",BI=6$;
			tmpc->speed	= 2500.0;
		}
		d = (i + 1) * track_head.last->d / CARS_NUM;
		while (tmpt->next != &track_head && tmpt->d < d)
			tmpt = tmpt->next;
	}
	distance = 0;
	tf = 0;
	t0 = tS;
	game_over = FALSE;
}

U0 CleanUp()
{
	while (mp_not_done_flags)
		Yield;
	QueueDel(&track_head, TRUE);
	DCDel(track_map);
}

U0 Varoom()
{
	I64 sc, last = counts.timer;

	Bool is_up_pressed;
	Bool is_down_pressed;
	Bool is_left_pressed;
	Bool is_right_pressed;
	Bool is_nl_pressed;
	Bool is_esc_pressed;

	Bool is_key_pressed;

	I64 sc_nl = Char2ScanCode('\n'); // scancode for typing newline

	MenuPush(
				"File {"
				"  Abort(,CH_SHIFT_ESC);"
				"  Exit(,CH_ESC);"
				"}"
				"Play {"
				"  Restart(,'\n');"
				"  Accelerator(,,SC_CURSOR_UP);"
				"  Brake(,,SC_CURSOR_DOWN);"
				"  Left(,,SC_CURSOR_LEFT);"
				"  Right(,,SC_CURSOR_RIGHT);"
				"}"
				);
	SettingsPush; //See $LK,"SettingsPush",A="MN:SettingsPush"$
	try
	{
		Fs->text_attr = YELLOW << 4 + BLUE;
		Fs->win_inhibit = WIG_TASK_DEFAULT - WIF_SELF_FOCUS - WIF_SELF_GRAB_SCROLL - WIF_FOCUS_TASK_MENU;
		AutoComplete;
		WinBorder;
		WinMax;
		DocCursor;
		DocClear;
		Init;
		PaletteSetLight(FALSE);
		Fs->draw_it = &DrawIt;
		Fs->animate_task = Spawn(&AnimateTask, NULL, "Animate",, Fs);
		while (TRUE)
		{
			is_up_pressed		= Bt(kbd.down_bitmap, SC_CURSOR_UP);
			is_down_pressed 	= Bt(kbd.down_bitmap, SC_CURSOR_DOWN);
			is_left_pressed 	= Bt(kbd.down_bitmap, SC_CURSOR_LEFT);
			is_right_pressed	= Bt(kbd.down_bitmap, SC_CURSOR_RIGHT);
			is_nl_pressed		= Bt(kbd.down_bitmap, sc_nl);
			is_esc_pressed		= Bt(kbd.down_bitmap, SC_ESC);

			if (is_left_pressed)	c[0].d�		-= .000003 * (counts.timer - last);
			if (is_right_pressed)	c[0].d�		+= .000003 * (counts.timer - last);
			if (is_up_pressed) 		c[0].speed  += .01 * (counts.timer - last);
			if (is_down_pressed)
			{
				c[0].speed -= .03 * (counts.timer - last);
				if (c[0].speed < 0)
					c[0].speed = 0;
			}
			if (is_nl_pressed)
			{
				CleanUp;
				Init;
			}
			if (is_esc_pressed)
				goto vr_done;

			last = counts.timer;

			LBts(&Fs->task_flags, TASKf_IDLE);
			Yield;
			LBtr(&Fs->task_flags, TASKf_IDLE);


/*			switch (KeyGet(&sc))
			{
				case 0:
					switch (sc.u8[0])
					{

						case SC_CURSOR_LEFT:
							c[0].d� -= � / 60;
							break;

						case SC_CURSOR_RIGHT:
							c[0].d� += � / 60;
							break;

						case SC_CURSOR_UP:
							c[0].speed += 300;
							break;

						case SC_CURSOR_DOWN:
							c[0].speed -= 900;
							if (c[0].speed < 0)
								c[0].speed = 0;
							break;
					}
					break;
				case '\n':
					CleanUp;
					Init;
					break;
				case CH_SHIFT_ESC:
				case CH_ESC:
					goto vr_done;
			}
*/
		}
vr_done: //Don't goto out of try
	}
	catch
		PutExcept;
	SettingsPop;
	FlushMessages; // if Varoom included in Cmd line, fixes SHIFT_ESC quitting current cmd line.
	mp_not_done_flags = 0; // fix for CleanUp hanging on a Yield while loop ...
	CleanUp;
	MenuPop;
	RegWrite("ZenithOS/Varoom", "F64 best_score=%5.4f;\n", best_score);
}

Varoom;
        �         
   ����    ����
    ����   ����
����������������
    ������������
   ����   ����
   ������������
    ����
   ����
   ����   ����
       ��������
   ����   ����
   ����        
    ������������
   ������������
����������������        �         
��������   ����
����       ������������   
   ����   ����
   ����   ����
   ����"   ����
   ����$   ����
   ����$   ����
����������������
����������������
��������   ����
"   ����&   ����
   ����   ����                

         ����
   ����      
   ����   ����
   ����   ����
   ����   ����
   ����   ����
   ����   ����
   ����   ����
   ����   ����

   ����    ����
   ����   ����
   ����   ����
   ����   ����
   ����   ����
   ����   ����
           ����        �        
   ����    ����
����������������
��������    ����
����������������
����������������
����������������
   ������������
��������
   ����

   ������������
   ����   ����
   ����   z���
   z���   ����
   ����    ����
    ����   v���
   v�������`���
����`�����������
����������������
����������������
����������������
����������������
����������������
����������������

   ����   ����
   ����   ����
������������j���
����j�����������
����������������
����������������
����������������

   ����   ����
��������    h���
   ����   ����
����~���   n���
    ~���   ����

   ����   ����
����������������
������������j���
����������������        J     `   �   F   ����}   -   ����}   -   ����K   F   ����K   7   �����   7   �����       �����   (   �����   (   ����A   7   ����A   7   ����        ����    F      }   -      }   -      K   F      K   7      �   7      �          �   (      �   (      A   7      A   7                     ��������}   ��������}   ��������K   ��������K   ���������   ���������       �����   ���������   ��������A   ��������A   ��������        ����    ����   }   ����   }   ����   K   ����   K   ����   �   ����   �          �   ����   �   ����   A   ����   A   ����                  F   ��������-   ��������-   ��������F   ��������7   ����y���7   ����j���    ����j���(   ����y���(   ��������7   ��������7   ����        ����    F      ����-      ����-      ����F      ����7      y���7      j���       j���(      y���(      ����7      ����7                     ��������������������������������������������������������y�����������j���    ����j�����������y�����������������������������������        ����    ����   ��������   ��������   ��������   ��������   y�������   j���       j�������   y�������   ��������   ��������                                                                                                                                                                                       
   	      	                                                                                                                                                                                    	               
   	                                                       $   %   &       &   '   $                                    %   $       $             $   '                $          %          %   &          &                '   &      #   !   "      !   #          #                                                  /   -   .      -   /   ,      /   *   ,      ,   *   +      +   )   (      +   *   )         +   (      +               +         +      (      !   .   -      .   !   "          +         +       ,         *   )      *             <   =   >       >   ?   <       1   0   3       3   2   1       1   =   <       <   0   1       <   ?   3       3   0   <       1   =   2       =   >   2       >   2   3       3   ?   >      ;   9   :      9   ;   8      8   6   7      7   5   4      7   6   5      G   E   F      E   G   D      G   B   D      D   B   C      C   A   @      C   B   A      4   C   @      C   4   7      4   C   7      C   4   @      9   F   E      F   9   :      8   C   7      C   8   D      5   B   A      B   5   6       T   U   V       V   W   T       I   H   K       K   J   I       I   U   T       T   H   I       T   W   K       K   H   T       I   U   J       U   V   J       V   J   K       K   W   V      S   R   Q      Q   P   S      P   O   N      O   L   M      O   M   N      _   ^   ]      ]   \   _      _   \   Z      \   [   Z      [   X   Y      [   Y   Z      L   X   [      [   O   L      L   O   [      [   X   L      Q   ]   ^      ^   R   Q      P   O   [      [   \   P      M   Y   Z      Z   N   M         P   8      8   P   6      5   4   @      @   A   5      L   M   Y      Y   X   L           J     `   �   F   ����}   -   ����}   -   ����K   F   ����K   7   �����   7   �����       �����   (   �����   (   ����A   7   ����A   7   ����        ����    F      }   -      }   -      K   F      K   7      �   7      �          �   (      �   (      A   7      A   7                     ��������}   ��������}   ��������K   ��������K   ���������   ���������       �����   ���������   ��������A   ��������A   ��������        ����    ����   }   ����   }   ����   K   ����   K   ����   �   ����   �          �   ����   �   ����   A   ����   A   ����                  F   ��������-   ��������-   ��������F   ��������7   ����y���7   ����j���    ����j���(   ����y���(   ��������7   ��������7   ����        ����    F      ����-      ����-      ����F      ����7      y���7      j���       j���(      y���(      ����7      ����7                     ��������������������������������������������������������y�����������j���    ����j�����������y�����������������������������������        ����    ����   ��������   ��������   ��������   ��������   y�������   j���       j�������   y�������   ��������   ��������                                                                                                                                                                                       
   	      	                                                                                                                                                                                    	               
   	                                                       $   %   &       &   '   $                                    %   $       $             $   '                $          %          %   &          &                '   &      #   !   "      !   #          #                                                  /   -   .      -   /   ,      /   *   ,      ,   *   +      +   )   (      +   *   )         +   (      +               +         +      (      !   .   -      .   !   "          +         +       ,         *   )      *             <   =   >       >   ?   <       1   0   3       3   2   1       1   =   <       <   0   1       <   ?   3       3   0   <       1   =   2       =   >   2       >   2   3       3   ?   >      ;   9   :      9   ;   8      8   6   7      7   5   4      7   6   5      G   E   F      E   G   D      G   B   D      D   B   C      C   A   @      C   B   A      4   C   @      C   4   7      4   C   7      C   4   @      9   F   E      F   9   :      8   C   7      C   8   D      5   B   A      B   5   6       T   U   V       V   W   T       I   H   K       K   J   I       I   U   T       T   H   I       T   W   K       K   H   T       I   U   J       U   V   J       V   J   K       K   W   V      S   R   Q      Q   P   S      P   O   N      O   L   M      O   M   N      _   ^   ]      ]   \   _      _   \   Z      \   [   Z      [   X   Y      [   Y   Z      L   X   [      [   O   L      L   O   [      [   X   L      Q   ]   ^      ^   R   Q      P   O   [      [   \   P      M   Y   Z      Z   N   M         P   8      8   P   6      5   4   @      @   A   5      L   M   Y      Y   X   L    