#define GRDIR_CLEAR		TRUE
//
//	Set to TRUE if you want to clear
//	the DolDoc before each dir listing,
//	to make it look and work more
//	like a modern file manager.


		$SP,"<1>",BI=1$		// <1> code files		(.CC)





		$SP,"<2>",BI=2$		// <2> header files		(.HH)






		$SP,"<3>",BI=3$		// <3> directories






		$SP,"<4>",BI=4$		// <4> disc files		(.iso, .ISO, .ISO.C)








		$SP,"<5>",BI=5$		// <5> DolDoc files		(.DD)







		$SP,"<6>",BI=6$		// <6> binary files		(.BIN, .BIN.C)








		$SP,"<7>",BI=7$		// <7> InFiles			(.IN)






		$SP,"<8>",BI=8$		// <8> text files		(.TXT)







		$SP,"<9>",BI=9$		// <9> unknown filetype









public CDocEntry *GrDirSprite(CDoc *doc=NULL, U8 *elems, U8 *filename=NULL, I64 align=-1)
{	// Draw a GrDir sprite into DolDoc: align arg is 0,1,2 for left,center,right.
	// make into either Link and PopUp, or Left-Macro,
	// so it can be left/right clicked just like the text links.
	// See $LK+PU,"DocSprite",A="FF:::/Zenith/Gr/GrSpritePlot.CC,DocSprite"$.

	I64			 size;
	U8			*st;
	Bool		 unlock;
	CDocEntry	*doc_e;
	CDocBin		*tmpb;

	if (!doc && !(doc = DocPut))
		return NULL;
	unlock = DocLock(doc);
	size = SpriteSize(elems);
	tmpb = CAlloc(sizeof(CDocBin), doc->mem_task);
	tmpb->size		= size;
	tmpb->data		= MAlloc(size, doc->mem_task);
	MemCopy(tmpb->data, elems, size);
	tmpb->num		= doc->cur_bin_num;
	tmpb->use_count = 1;
	QueueInsert(tmpb, doc->bin_head.last);


	if (align == 0)
		"\n$$CM+LX-RE,LE=6$$";
	else if (align == 1)
		"$$CM+CX-RE,LE=2$$";
	else if (align == 2)
		"$$CM+RX-RE,LE=-1$$";


	if (!IsDir(filename))
		st = MStrPrint("$$SP+L+PU,\"\",A=\"%s\",BI=%d$$", filename, doc->cur_bin_num++);
	else
		st = MStrPrint("$$SP+X,\"\",LM=\"Cd(\\\"%s\\\");GrDir;\n\",BI=%d$$", filename, doc->cur_bin_num++);

	doc_e = DocPrint(doc, "%s", st);
	Free(st);
	doc_e->bin_data = tmpb;
	if (doc_e && doc_e->de_flags & DOCEF_TAG && doc_e->tag && *doc_e->tag)
		tmpb->tag = StrNew(doc_e->tag, doc->mem_task);
	if (unlock)
		DocUnlock(doc);

	return doc_e;
}

I64 GrDir(U8 *files_find_mask=NULL)
{
	CDirEntry	*tmpde1 = NULL, *tmpde2;
	I64			 res	= 0;
	U8			*tag, *st,
				*tmpb	= NULL,
				*tmpb0	= NULL, *tmpb1 = NULL, *tmpb2 = NULL, // sprite pointers
				*tmpf0	= NULL, *tmpf1 = NULL, *tmpf2 = NULL; // filenames (for sprite links)

	tmpde1 = FilesFind(files_find_mask);
	if (tmpde1)
	{

		if (GRDIR_CLEAR)
			"$$CL$$";

		st = MAllocIdent(tmpde1->full_name);
		StrLastRemove(st, "/");
		if (!st[2])
			StrCopy(st + 2, "/");

		"\n$$MA,T=\"Directory\",LM=\"PopUpCd;GrDir;\n\"$$ of %s\n\n", st;

		while (tmpde1)
		{
			tmpde2 = tmpde1->next;

			switch (res % 3)
			{
				case 0:
					tag = "LX";
					break;
				case 1:
					tag = "CX";
					break;
				case 2:
					tag = "RX";
					break;
			}

			if (res % 3 == 0 && res != 0)
			{
				GrDirSprite(, tmpb0, tmpf0, 0);
				GrDirSprite(, tmpb1, tmpf1, 1);
				GrDirSprite(, tmpb2, tmpf2, 2);

				"\n\n\n\n\n\n\n";
				tmpb  = NULL;
				tmpb0 = NULL;
				tmpb1 = NULL;
				tmpb2 = NULL;

				Free(tmpf0);
				Free(tmpf1);
				Free(tmpf2);
				tmpf0 = NULL;
				tmpf1 = NULL;
				tmpf2 = NULL;
			}

			if (res % 3 == 2)
				"$$CM-LE-RE$$";

			if (tmpde1->attr & RS_ATTR_DIR)
			{

				"$$MA+%s,T=\"%s\",LM=\"Cd(\\\"%s\\\");GrDir;\n\"$$", tag, tmpde1->name, tmpde1->full_name;
				switch (res % 3)
				{
					case 0:
						tmpb0 = $IB,"<3>",BI=3$;
						tmpf0 = StrNew(tmpde1->full_name);
						break;
					case 1:
						tmpb1 = $IB,"<3>",BI=3$;
						tmpf1 = StrNew(tmpde1->full_name);
						break;
					case 2:
						tmpb2 = $IB,"<3>",BI=3$;
						tmpf2 = StrNew(tmpde1->full_name);
						break;
				}

			}
			else
			{
				"$$LK+%s+PU,\"%s\",A=\"FI:%s\"$$", tag, tmpde1->name, tmpde1->full_name;
				if (!StrCompare(FileExtDot(tmpde1->name), ".CC"))
					tmpb = $IB,"<1>",BI=1$;
				else if (!StrCompare(FileExtDot(tmpde1->name), ".DD"))
					tmpb = $IB,"<5>",BI=5$;
				else if (!StrCompare(FileExtDot(tmpde1->name), ".ISO.C")	||
						 !StrCompare(FileExtDot(tmpde1->name), ".ISO")		||
						 !StrCompare(FileExtDot(tmpde1->name), ".iso"))
					tmpb = $IB,"<4>",BI=4$;
				else if (!StrCompare(FileExtDot(tmpde1->name), ".HH"))
					tmpb = $IB,"<2>",BI=2$;
				else if (!StrCompare(FileExtDot(tmpde1->name), ".BIN.C")	||
						 !StrCompare(FileExtDot(tmpde1->name), ".BIN"))
					tmpb = $IB,"<6>",BI=6$;
				else if (!StrCompare(FileExtDot(tmpde1->name), ".IN"))
					tmpb = $IB,"<7>",BI=7$;
				else if (!StrCompare(FileExtDot(tmpde1->name), ".TXT"))
					tmpb = $IB,"<8>",BI=8$;
				else
					// sprite for unknown filetype
					tmpb = $IB,"<9>",BI=9$;

				switch (res % 3)
				{
					case 0:
						tmpb0 = tmpb;
						tmpf0 = StrNew(tmpde1->full_name);
						break;
					case 1:
						tmpb1 = tmpb;
						tmpf1 = StrNew(tmpde1->full_name);
						break;
					case 2:
						tmpb2 = tmpb;
						tmpf2 = StrNew(tmpde1->full_name);
						break;
				}
			}

			res++;
			DirEntryDel(tmpde1);
			tmpde1 = tmpde2;
		}

		Free(st);
	}
	else
		"\nNo matching entries!!\n";

	GrDirSprite(, tmpb0, tmpf0, 0);
	GrDirSprite(, tmpb1, tmpf1, 1);
	GrDirSprite(, tmpb2, tmpf2, 2);
	"\n\n\n\n\n\n\n";


	return res;
}

GrDir;        :      ����   ����%   ����   ����    ����   ����           8      ����   ����$   ����   ����    ����   ����           	     ����    0   0   ������������������������	�����������������������������������������												���������������������������������																		����������������������������																						������������������������																										���������������������																													�����������������																															����������������																																		��������������																																		������������																																					�����������																																						���������																																								��������																																								�������																																										������																																										�����																																												����																																												���																																					���																																		��																																		��																																�																															�																															�																																																																																													��																														��																														��																														��																																��																																		���																																	����																																				����																																										�����																																										������																																										�������																																								��������																																							�����������																																					�����������																																				�������������																																	����������������																																�����������������																													���������������������																										������������������������																							��������������������������																				�������������������������������														���������������������������������������				����������������������        �      ����       *          � ����   ����,          �����   ����1          �   
����   ����   
����   ����   
����   ����            ;      ����   ����%   ����   ����    ����   ����           �      ����   ����4   ����         ��E��U	@����         ��E��U	@����         ��E��U	@����   ����   ����   ����                 :      ����   ����%   ����   ����    ����   ����           �      ����       *          � ����   ����,          �����   ����1          �   
����   ����   
����   ����   
����   ����     	       �      ����       *          � ����   ����,          �����   ����1          �   
����   ����   
����   ����   
����   ����     