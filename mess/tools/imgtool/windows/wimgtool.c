//============================================================
//
//	wimgtool.c - Win32 GUI Imgtool
//
//============================================================

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <tchar.h>

#include "wimgtool.h"
#include "wimgres.h"
#include "pile.h"
#include "pool.h"
#include "strconv.h"
#include "attrdlg.h"

const TCHAR wimgtool_class[] = TEXT("wimgtool_class");
const TCHAR wimgtool_producttext[] = TEXT("MESS Image Tool");

extern void win_association_dialog(HWND parent, imgtool_library *library);

struct wimgtool_info
{
	HWND listview;
	HWND statusbar;
	imgtool_image *image;
	char *filename;
	int open_mode;
	char *current_directory;

	HIMAGELIST iconlist_normal;
	HIMAGELIST iconlist_small;
	mess_pile iconlist_extensions;

	HICON readonly_icon;
	int readonly_icon_index;
	int directory_icon_index;

	HIMAGELIST dragimage;
	POINT dragpt;
};



static void nyi(HWND window)
{
	MessageBox(window, TEXT("Not yet implemented"), wimgtool_producttext, MB_OK);
}



static struct wimgtool_info *get_wimgtool_info(HWND window)
{
	struct wimgtool_info *info;
	LONG_PTR l;
	l = GetWindowLongPtr(window, GWLP_USERDATA);
	info = (struct wimgtool_info *) l;
	return info;
}



static int get_selected_item(HWND window)
{
	struct wimgtool_info *info;
	LVITEM item;
	int selected_index;

	info = get_wimgtool_info(window);
	if (!info->image)
		return -1;

	selected_index = ListView_GetNextItem(info->listview, -1, LVIS_SELECTED | LVIS_FOCUSED);
	if (selected_index < 0)
		return -1;

	item.mask = LVIF_PARAM;
	item.iItem = selected_index;
	ListView_GetItem(info->listview, &item);

	if (item.lParam < 0)
		return -1;

	return item.lParam;
}



static imgtoolerr_t get_selected_dirent(HWND window, imgtool_dirent *entry)
{
	struct wimgtool_info *info;
	int selected_item;
	imgtoolerr_t err;
	size_t filename_sz, curdir_sz;
	char *s;
	
	info = get_wimgtool_info(window);

	selected_item = get_selected_item(window);
	if (selected_item < 0)
	{
		err = IMGTOOLERR_UNEXPECTED;
		goto done;
	}

	/* retrieve the directory entry */
	err = img_getdirent(info->image, info->current_directory, selected_item, entry);
	if (err)
		goto done;

	/* if we have a path, prepend the path */
	if (info->current_directory && info->current_directory[0])
	{
		curdir_sz = strlen(info->current_directory);
		filename_sz = strlen(entry->filename);

		s = (char *) alloca((curdir_sz + filename_sz + 1) * sizeof(*s));
		strcpy(s, info->current_directory);
		strcpy(s + curdir_sz, entry->filename);
		snprintf(entry->filename, sizeof(entry->filename) / sizeof(entry->filename[0]), "%s", s);
	}

done:
	return err;
}



static void report_error(HWND window, imgtoolerr_t err)
{
	MessageBox(window, imgtool_error(err), wimgtool_producttext, MB_OK);
}



#define FOLDER_ICON	((const char *) ~0)

static int append_associated_icon(HWND window, const char *extension)
{
	HICON icon;
	HANDLE file = INVALID_HANDLE_VALUE;
	WORD icon_index;
	TCHAR file_path[MAX_PATH];
	int index = -1;
	struct wimgtool_info *info;

	info = get_wimgtool_info(window);

	/* retrieve temporary file path */
	GetTempPath(sizeof(file_path) / sizeof(file_path[0]), file_path);

	if (extension != FOLDER_ICON)
	{
		/* create bogus temporary file so that we can get the icon */
		_tcscat(file_path, TEXT("tmp"));
		if (extension)
			_tcscat(file_path, A2T(extension));

		file = CreateFile(file_path, GENERIC_WRITE, 0, NULL, CREATE_NEW, 0, NULL);
	}

	/* extract the icon */
	icon = ExtractAssociatedIcon(GetModuleHandle(NULL), file_path, &icon_index);
	if (icon)
	{
		index = ImageList_AddIcon(info->iconlist_normal, icon);
		ImageList_AddIcon(info->iconlist_small, icon);
		DestroyIcon(icon);
	}

	/* remote temporary file if we created one */
	if (file != INVALID_HANDLE_VALUE)
	{
		CloseHandle(file);
		DeleteFile(file_path);
	}

	return index;
}



static imgtoolerr_t append_dirent(HWND window, int index, const imgtool_dirent *entry)
{
	LVITEM lvi;
	int new_index, column_index;
	struct wimgtool_info *info;
	TCHAR buffer[32];
	int icon_index;
	const char *extension;
	const char *ptr;
	size_t size, i;
	struct imgtool_module_features features;
	struct tm *local_time;

	info = get_wimgtool_info(window);
	features = img_get_module_features(img_module(info->image));

	if (entry->directory)
	{
		icon_index = info->directory_icon_index;
	}
	else
	{
		extension = strrchr(entry->filename, '.');
		if (!extension)
			extension = ".bin";

		ptr = pile_getptr(&info->iconlist_extensions);
		size = pile_size(&info->iconlist_extensions);
		icon_index = 2;
		for (i = 0; i < size; i += strlen(&ptr[i]) + 1)
		{
			if (!stricmp(&ptr[i], extension))
				break;
			icon_index++;
		}

		if (i >= size)
		{
			icon_index = append_associated_icon(window, extension);
			if (icon_index < 0)
				return IMGTOOLERR_UNEXPECTED;
			if (pile_puts(&info->iconlist_extensions, extension))
				return IMGTOOLERR_OUTOFMEMORY;
			if (pile_putc(&info->iconlist_extensions, '\0'))
				return IMGTOOLERR_OUTOFMEMORY;
		}
	}

	memset(&lvi, 0, sizeof(lvi));
	lvi.iItem = ListView_GetItemCount(info->listview);
	lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
	lvi.pszText = U2T((char *) entry->filename);
	lvi.iImage = icon_index;
	lvi.lParam = index;
	new_index = ListView_InsertItem(info->listview, &lvi);

	if (entry->directory)
	{
		_sntprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), TEXT("<DIR>"));
	}
	else
	{
		_sntprintf(buffer, sizeof(buffer) / sizeof(buffer[0]),
			TEXT("%d"), entry->filesize);
	}
	column_index = 1;
	ListView_SetItemText(info->listview, new_index, column_index++, buffer);

	if (features.supports_creation_time)
	{
		local_time = localtime(&entry->creation_time);
		_sntprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), _tasctime(local_time));
		rtrim(buffer);
		ListView_SetItemText(info->listview, new_index, column_index++, buffer);
	}

	if (features.supports_lastmodified_time)
	{
		local_time = localtime(&entry->lastmodified_time);
		_sntprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), _tasctime(local_time));
		rtrim(buffer);
		ListView_SetItemText(info->listview, new_index, column_index++, buffer);
	}

	if (entry->attr)
		ListView_SetItemText(info->listview, new_index, column_index++, U2T((char *) entry->attr));
	if (entry->corrupt)
		ListView_SetItemText(info->listview, new_index, column_index++, (LPTSTR) TEXT("Corrupt"));
	return 0;
}



static imgtoolerr_t refresh_image(HWND window)
{
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;
	struct wimgtool_info *info;
	imgtool_imageenum *imageenum = NULL;
	char size_buf[32];
	imgtool_dirent entry;
	UINT64 filesize;
	int i;
	BOOL is_root_directory;
	struct imgtool_module_features features;

	info = get_wimgtool_info(window);
	size_buf[0] = '\0';

	ListView_DeleteAllItems(info->listview);

	if (info->image)
	{
		memset(&features, 0, sizeof(features));
		features = img_get_module_features(img_module(info->image));

		is_root_directory = TRUE;
		if (info->current_directory)
		{
			for (i = 0; info->current_directory[i]; i++)
			{
				if (info->current_directory[i] != img_module(info->image)->path_separator)
				{
					is_root_directory = FALSE;
					break;
				}
			}
		}

		memset(&entry, 0, sizeof(entry));

		if (!is_root_directory)
		{
			strcpy(entry.filename, "..");
			entry.directory = 1;
			entry.attr[0] = '\0';
			err = append_dirent(window, -1, &entry);
			if (err)
				goto done;
		}


		err = img_beginenum(info->image, info->current_directory, &imageenum);
		if (err)
			goto done;

		i = 0;
		do
		{
			err = img_nextenum(imageenum, &entry);
			if (err)
				goto done;

			if (entry.filename[0])
			{
				err = append_dirent(window, i++, &entry);
				if (err)
					goto done;
			}
		}
		while(!entry.eof);

		if (features.supports_freespace)
		{
			err = img_freespace(info->image, &filesize);
			if (err)
				goto done;
			snprintf(size_buf, sizeof(size_buf) / sizeof(size_buf[0]), "%u bytes free", (unsigned) filesize);
		}

	}
	SendMessage(info->statusbar, SB_SETTEXT, 2, (LPARAM) U2T(size_buf));

done:
	if (imageenum)
		img_closeenum(imageenum);
	return err;
}



static imgtoolerr_t full_refresh_image(HWND window)
{
	struct wimgtool_info *info;
	LVCOLUMN col;
	const struct ImageModule *module;
	int column_index = 0;
	int i;
	char buf[256];
	TCHAR file_title[MAX_PATH];
	const char *statusbar_text[2];
	struct imgtool_module_features features;

	info = get_wimgtool_info(window);

	// get the modules and features
	module = info->image ? img_module(info->image) : NULL;
	if (module)
		features = img_get_module_features(module);
	else
		memset(&features, 0, sizeof(features));

	if (info->filename)
	{
		GetFileTitle(U2T(info->filename), file_title, sizeof(file_title) / sizeof(file_title[0]));

		snprintf(buf, sizeof(buf) / sizeof(buf[0]),
			(info->current_directory && info->current_directory[0]) ? "%s - %s" : "%s",
			T2U(file_title), info->current_directory);
		statusbar_text[0] = osd_basename((char *) info->filename);
		statusbar_text[1] = module->description;
	}
	else
	{
		snprintf(buf, sizeof(buf) / sizeof(buf[0]),
			"%s %s", wimgtool_producttext, build_version);
		statusbar_text[0] = NULL;
		statusbar_text[1] = NULL;
	}
	SetWindowText(window, U2T(buf));
	for (i = 0; i < sizeof(statusbar_text) / sizeof(statusbar_text[0]); i++)
		SendMessage(info->statusbar, SB_SETTEXT, i, (LPARAM) U2T(statusbar_text[i]));

	// set the icon
	if (info->image && (info->open_mode == OSD_FOPEN_READ))
		SendMessage(info->statusbar, SB_SETICON, 0, (LPARAM) info->readonly_icon);
	else
		SendMessage(info->statusbar, SB_SETICON, 0, (LPARAM) 0);

	DragAcceptFiles(window, info->filename != NULL);

	// create the listview columns
	col.mask = LVCF_TEXT | LVCF_WIDTH;
	col.cx = 200;
	col.pszText = (LPTSTR) TEXT("Filename");
	if (ListView_InsertColumn(info->listview, column_index++, &col) < 0)
		return IMGTOOLERR_OUTOFMEMORY;

	col.cx = 60;
	col.pszText = (LPTSTR) TEXT("Size");
	if (ListView_InsertColumn(info->listview, column_index++, &col) < 0)
		return IMGTOOLERR_OUTOFMEMORY;

	if (features.supports_creation_time)
	{
		col.cx = 160;
		col.pszText = (LPTSTR) TEXT("Creation time");
		if (ListView_InsertColumn(info->listview, column_index++, &col) < 0)
			return IMGTOOLERR_OUTOFMEMORY;
	}

	if (features.supports_lastmodified_time)
	{
		col.cx = 160;
		col.pszText = (LPTSTR) TEXT("Last modified time");
		if (ListView_InsertColumn(info->listview, column_index++, &col) < 0)
			return IMGTOOLERR_OUTOFMEMORY;
	}

	col.cx = 60;
	col.pszText = (LPTSTR) TEXT("Attributes");
	if (ListView_InsertColumn(info->listview, column_index++, &col) < 0)
		return IMGTOOLERR_OUTOFMEMORY;

	col.cx = 60;
	col.pszText = (LPTSTR) TEXT("Notes");
	if (ListView_InsertColumn(info->listview, column_index++, &col) < 0)
		return IMGTOOLERR_OUTOFMEMORY;

	// delete extraneous columns
	while(ListView_DeleteColumn(info->listview, column_index))
		;
	return refresh_image(window);
}



static imgtoolerr_t setup_openfilename_struct(OPENFILENAME *ofn, memory_pool *pool,
	HWND window, BOOL creating_file)
{
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;
	mess_pile pile;
	const struct ImageModule *default_module = NULL;
	const struct ImageModule *module = NULL;
	const char *s;
	TCHAR *filename;
	TCHAR *filter;
	struct imgtool_module_features features;
	DWORD filter_index = 0, current_index = 0;
	const struct wimgtool_info *info;

	info = get_wimgtool_info(window);
	if (info->image)
		default_module = img_module(info->image);

	memset(ofn, 0, sizeof(*ofn));
	pile_init(&pile);

	if (!creating_file)
	{
		current_index++;
		pile_puts(&pile, "Autodetect (*.*)");
		pile_putc(&pile, '\0');
		pile_puts(&pile, "*.*");
		pile_putc(&pile, '\0');
	}

	// write out library modules
	while((module = imgtool_library_iterate(library, module)) != NULL)
	{
		// check to see if we have the right features
		features = img_get_module_features(module);
		if (creating_file ? features.supports_create : features.supports_open)
		{
			// is this the filter we are asking for?
			current_index++;
			if (module == default_module)
				filter_index = current_index;

			pile_puts(&pile, module->description);
			pile_puts(&pile, " (");

			s = module->extensions;
			while(*s)
			{
				if (s != module->extensions)
					pile_putc(&pile, ';');
				pile_printf(&pile, "*.%s", s);
				s += strlen(s) + 1;
			}
			pile_putc(&pile, ')');
			pile_putc(&pile, '\0');

			s = module->extensions;
			while(*s)
			{
				if (s != module->extensions)
					pile_putc(&pile, ';');
				pile_printf(&pile, "*.%s", s);
				s += strlen(s) + 1;
			}

			pile_putc(&pile, '\0');
		}
	}
	pile_putc(&pile, '\0');
	pile_putc(&pile, '\0');

	filename = pool_malloc(pool, sizeof(TCHAR) * MAX_PATH);
	if (!filename)
	{
		err = IMGTOOLERR_OUTOFMEMORY;
		goto done;
	}
	filename[0] = '\0';

	filter = pool_malloc(pool, pile_size(&pile));
	if (!filter)
	{
		err = IMGTOOLERR_OUTOFMEMORY;
		goto done;
	}
	memcpy(filter, pile_getptr(&pile), pile_size(&pile));

	ofn->lStructSize = sizeof(*ofn);
	ofn->Flags = OFN_EXPLORER;
	ofn->hwndOwner = window;
	ofn->lpstrFile = filename;
	ofn->nMaxFile = MAX_PATH;
	ofn->lpstrFilter = filter;
	ofn->nFilterIndex = filter_index;

done:
	pile_delete(&pile);
	return err;
}



const struct ImageModule *find_filter_module(int filter_index,
	BOOL creating_file)
{
	const struct ImageModule *module = NULL;
	struct imgtool_module_features features;

	if (filter_index-- == 0)
		return NULL;
	if (!creating_file && (filter_index-- == 0))
		return NULL;

	while((module = imgtool_library_iterate(library, module)) != NULL)
	{
		features = img_get_module_features(module);
		if (creating_file ? features.supports_create : features.supports_open)
		{
			if (filter_index-- == 0)
				return module;
		}
	}
	return NULL;
}



imgtoolerr_t wimgtool_open_image(HWND window, const struct ImageModule *module,
	const char *filename, int read_or_write)
{
	imgtoolerr_t err;
	imgtool_image *image;
	struct wimgtool_info *info;
	struct imgtool_module_features features;
	char buf[2];

	info = get_wimgtool_info(window);

	/* if the module is not specified, auto detect the format */
	if (!module)
	{
		err = img_identify(library, filename, &module, 1);
		if (err)
			goto done;
	}

	/* check to see if this module actually supports writing */
	if (read_or_write != OSD_FOPEN_READ)
	{
		features = img_get_module_features(module);
		if (!features.supports_writing && !features.supports_createdir && !features.supports_deletefile && !features.supports_deletedir)
			read_or_write = OSD_FOPEN_READ;
	}

	info->filename = strdup(filename);
	if (!info->filename)
	{
		err = IMGTOOLERR_OUTOFMEMORY;
		goto done;
	}
	
	err = img_open(module, filename, read_or_write, &image);
	if ((ERRORCODE(err) == IMGTOOLERR_READONLY) && read_or_write)
	{
		/* if we failed when open a read/write image, try again */
		read_or_write = OSD_FOPEN_READ;
		err = img_open(module, filename, read_or_write, &image);
	}
	if (err)
		goto done;

	if (info->image)
		img_close(info->image);
	info->image = image;
	info->open_mode = read_or_write;
	if (info->current_directory)
	{
		free(info->current_directory);
		info->current_directory = NULL;
	}

	// do we support directories?
	if (features.supports_directories)
	{
		if (module->initial_path_separator)
		{
			buf[0] = module->path_separator;
			buf[1] = '\0';
		}
		else
		{
			buf[0] = '\0';
		}
		info->current_directory = strdup(buf);
		if (!info->current_directory)
		{
			err = IMGTOOLERR_OUTOFMEMORY;
			goto done;
		}
	}

	// refresh the window
	full_refresh_image(window);
	
done:
	return err;
}



static void menu_new(HWND window)
{
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;
	memory_pool pool;
	OPENFILENAME ofn;
	const struct ImageModule *module;
	const char *filename;
	option_resolution *resolution = NULL;

	pool_init(&pool);

	err = setup_openfilename_struct(&ofn, &pool, window, TRUE);
	if (err)
		goto done;
	ofn.Flags |= OFN_ENABLETEMPLATE | OFN_ENABLEHOOK;
	ofn.hInstance = GetModuleHandle(NULL);
	ofn.lpTemplateName = MAKEINTRESOURCE(IDD_FILETEMPLATE);
	ofn.lpfnHook = win_new_dialog_hook;
	ofn.lCustData = (LPARAM) &resolution;
	if (!GetSaveFileName(&ofn))
		goto done;

	filename = T2A(ofn.lpstrFile);

	module = find_filter_module(ofn.nFilterIndex, TRUE);
	
	err = img_create(module, filename, resolution, NULL);
	if (err)
		goto done;

	err = wimgtool_open_image(window, module, filename, OSD_FOPEN_RW);
	if (err)
		goto done;

done:
	if (err)
		report_error(window, err);
	if (resolution)
		option_resolution_close(resolution);
	pool_exit(&pool);
}



static void menu_open(HWND window)
{
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;
	memory_pool pool;
	OPENFILENAME ofn;
	const struct ImageModule *module;
	const char *filename;
	struct wimgtool_info *info;
	int read_or_write;

	pool_init(&pool);

	info = get_wimgtool_info(window);

	err = setup_openfilename_struct(&ofn, &pool, window, FALSE);
	if (err)
		goto done;
	ofn.Flags |= OFN_FILEMUSTEXIST;
	if (!GetOpenFileName(&ofn))
		goto done;

	filename = T2A(ofn.lpstrFile);
	module = find_filter_module(ofn.nFilterIndex, FALSE);

	// is this file read only?
	if ((ofn.Flags & OFN_READONLY) || (GetFileAttributes(ofn.lpstrFile) & FILE_ATTRIBUTE_READONLY))
		read_or_write = OSD_FOPEN_READ;
	else
		read_or_write = OSD_FOPEN_RW;

	err = wimgtool_open_image(window, module, filename, read_or_write);
	if (err)
		goto done;

done:
	if (err)
		report_error(window, err);
	pool_exit(&pool);
}



static void menu_insert(HWND window)
{
	imgtoolerr_t err;
	const char *image_filename;
	TCHAR host_filename[MAX_PATH] = { 0 };
	const TCHAR *s1;
	char *s2;
	OPENFILENAME ofn;
	struct wimgtool_info *info;
	option_resolution *opts = NULL;
	BOOL cancel;
	const struct ImageModule *module;

	info = get_wimgtool_info(window);

	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFile = host_filename;
	ofn.nMaxFile = sizeof(host_filename) / sizeof(host_filename[0]);
	if (!GetOpenFileName(&ofn))
	{
		err = 0;
		goto done;
	}

	module = img_module(info->image);
	if (module->writefile_optguide && module->writefile_optspec)
	{
		err = win_show_option_dialog(window, module->writefile_optguide,
			module->writefile_optspec, &opts, &cancel);
		if (err || cancel)
			goto done;
	}

	s1 = _tcsrchr(ofn.lpstrFile, '\\');
	s1 = s1 ? s1 + 1 : ofn.lpstrFile;
	image_filename = T2U(s1);

	if (info->current_directory)
	{
		s2 = (char *) alloca(strlen(info->current_directory) + strlen(image_filename) + 1);
		strcpy(s2, info->current_directory);
		strcat(s2, image_filename);
		image_filename = s2;
	}

	err = img_putfile(info->image, image_filename, ofn.lpstrFile, opts, NULL);
	if (err)
		goto done;

	err = refresh_image(window);
	if (err)
		goto done;

done:
	if (opts)
		option_resolution_close(opts);
	if (err)
		report_error(window, err);
}



static void menu_extract(HWND window)
{
	imgtoolerr_t err;
	imgtool_dirent entry;
	TCHAR host_filename[MAX_PATH];
	OPENFILENAME ofn;
	struct wimgtool_info *info;
	const char *filename;
	const char *image_basename;
	int i;

	info = get_wimgtool_info(window);

	memset(&entry, 0, sizeof(entry));
	err = get_selected_dirent(window, &entry);
	if (err)
		goto done;
	filename = entry.filename;

	image_basename = entry.filename;
	for (i = 0; entry.filename[i]; i++)
	{
		if (entry.filename[i] == img_module(info->image)->path_separator)
			image_basename = &entry.filename[i + 1];
	}

	_tcscpy(host_filename, U2T(image_basename));

	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFile = host_filename;
	ofn.lpstrFilter = TEXT("All files (*.*)\0*.*\0");
	ofn.nMaxFile = sizeof(host_filename) / sizeof(host_filename[0]);
	if (!GetSaveFileName(&ofn))
		goto done;

	err = img_getfile(info->image, filename, ofn.lpstrFile, NULL);
	if (err)
		goto done;

done:
	if (err)
		report_error(window, err);
}



struct createdir_dialog_info
{
	HWND ok_button;
	HWND edit_button;
	TCHAR buf[256];
};



static INT_PTR CALLBACK createdir_dialog_proc(HWND dialog, UINT message, WPARAM wparam, LPARAM lparam)
{
	struct createdir_dialog_info *info;
	LONG_PTR l;
	INT_PTR rc = 0;
	int id;

	switch(message)
	{
		case WM_INITDIALOG:
			EnableWindow(GetDlgItem(dialog, IDOK), FALSE);
			SetWindowLongPtr(dialog, GWLP_USERDATA, lparam);
			info = (struct createdir_dialog_info *) lparam;

			info->ok_button = GetDlgItem(dialog, IDOK);
			info->edit_button = GetDlgItem(dialog, IDC_EDIT);
			break;

		case WM_COMMAND:
			l = GetWindowLongPtr(dialog, GWLP_USERDATA);
			info = (struct createdir_dialog_info *) l;

			switch(HIWORD(wparam))
			{
				case BN_CLICKED:
					id = LOWORD(wparam);
					if (id == IDCANCEL)
						info->buf[0] = '\0';
					EndDialog(dialog, id);
					break;

				case EN_CHANGE:
					GetWindowText(info->edit_button,
						info->buf, sizeof(info->buf) / sizeof(info->buf[0]));
					EnableWindow(info->ok_button, info->buf[0] != '\0');
					break;
			}
			break;
	}
	return rc;
}



static void menu_createdir(HWND window)
{
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;
	struct createdir_dialog_info cdi;
	struct wimgtool_info *info;
	char *dirname;
	char *s;

	info = get_wimgtool_info(window);

	memset(&cdi, 0, sizeof(cdi));
	DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_CREATEDIR),
		window, createdir_dialog_proc, (LPARAM) &cdi);

	if (cdi.buf[0] == '\0')
		goto done;
	dirname = T2U(cdi.buf);

	if (info->current_directory)
	{
		s = (char *) alloca(strlen(info->current_directory) + strlen(dirname) + 1);
		strcpy(s, info->current_directory);
		strcat(s, dirname);
		dirname = s;
	}

	err = img_createdir(info->image, dirname);
	if (err)
		goto done;

	err = refresh_image(window);
	if (err)
		goto done;

done:
	if (err)
		report_error(window, err);
}



static void menu_delete(HWND window)
{
	imgtoolerr_t err;
	imgtool_dirent entry;
	struct wimgtool_info *info;

	info = get_wimgtool_info(window);

	memset(&entry, 0, sizeof(entry));
	err = get_selected_dirent(window, &entry);
	if (err)
		goto done;

	if (entry.directory)
		err = img_deletedir(info->image, entry.filename);
	else
		err = img_deletefile(info->image, entry.filename);
	if (err)
		goto done;

	err = refresh_image(window);
	if (err)
		goto done;

done:
	if (err)
		report_error(window, err);
}



static void set_listview_style(HWND window, DWORD style)
{
	struct wimgtool_info *info;

	info = get_wimgtool_info(window);
	style &= LVS_TYPEMASK;
	style |= (GetWindowLong(info->listview, GWL_STYLE) & ~LVS_TYPEMASK);
	SetWindowLong(info->listview, GWL_STYLE, style);
}



static LRESULT wimgtool_create(HWND window, CREATESTRUCT *pcs)
{
	struct wimgtool_info *info;
	static int status_widths[3] = { 200, 400, -1 };

	info = malloc(sizeof(*info));
	if (!info)
		return -1;
	memset(info, 0, sizeof(*info));
	pile_init(&info->iconlist_extensions);

	SetWindowLongPtr(window, GWLP_USERDATA, (LONG_PTR) info);

	// create the list view
	info->listview = CreateWindow(WC_LISTVIEW, NULL,
		WS_VISIBLE | WS_CHILD, 0, 0, pcs->cx, pcs->cy, window, NULL, NULL, NULL);
	if (!info->listview)
		return -1;
	set_listview_style(window, LVS_REPORT);

	// create the status bar
	info->statusbar = CreateWindow(STATUSCLASSNAME, NULL, WS_VISIBLE | WS_CHILD,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, window, NULL, NULL, NULL);
	if (!info->statusbar)
		return -1;
	SendMessage(info->statusbar, SB_SETPARTS, sizeof(status_widths) / sizeof(status_widths[0]),
		(LPARAM) status_widths);

	// create imagelists
	info->iconlist_normal = ImageList_Create(32, 32, ILC_COLORDDB | ILC_MASK , 0, 0);
	info->iconlist_small = ImageList_Create(16, 16, ILC_COLORDDB | ILC_MASK , 0, 0);
	if (!info->iconlist_normal || !info->iconlist_small)
		return -1;
	ListView_SetImageList(info->listview, info->iconlist_normal, LVSIL_NORMAL);
	ListView_SetImageList(info->listview, info->iconlist_small, LVSIL_SMALL);

	// get icons
	info->readonly_icon = LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_READONLY), IMAGE_ICON, 16, 16, 0);
	info->readonly_icon_index = ImageList_AddIcon(info->iconlist_normal, info->readonly_icon);
	ImageList_AddIcon(info->iconlist_small, info->readonly_icon);
	info->directory_icon_index = append_associated_icon(window, FOLDER_ICON);

	full_refresh_image(window);
	return 0;
}



static void wimgtool_destroy(HWND window)
{
	struct wimgtool_info *info;

	info = get_wimgtool_info(window);

	if (info)
	{
		if (info->image)
			img_close(info->image);
		pile_delete(&info->iconlist_extensions);
		DestroyIcon(info->readonly_icon);
		if (info->current_directory)
			free(info->current_directory);
		free(info);
	}
}



static void drop_files(HWND window, HDROP drop)
{
	struct wimgtool_info *info;
	UINT count, i;
	TCHAR buffer[MAX_PATH];
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;

	info = get_wimgtool_info(window);

	count = DragQueryFile(drop, 0xFFFFFFFF, NULL, 0);
	for (i = 0; i < count; i++)
	{
		DragQueryFile(drop, i, buffer, sizeof(buffer) / sizeof(buffer[0]));

		err = img_putfile(info->image, NULL, buffer, NULL, NULL);
		if (err)
			goto done;
	}

done:
	refresh_image(window);
	if (err)
		report_error(window, err);
}



static imgtoolerr_t change_directory(HWND window, const char *dir)
{
	struct wimgtool_info *info;
	char *new_current_dir;
	char buf[2];
	char path_separator;
	int i;

	info = get_wimgtool_info(window);
	path_separator = img_module(info->image)->path_separator;

	if (!strcmp(dir, ".."))
	{
		i = strlen(info->current_directory);
		if (i > 0)
			i--;
		while((i > 0) && (info->current_directory[i-1] != path_separator))
			i--;
		info->current_directory[i] = '\0';
	}
	else
	{
		new_current_dir = realloc(info->current_directory,
			strlen(info->current_directory) + 1 + strlen(dir) + 1);
		if (!new_current_dir)
			return IMGTOOLERR_OUTOFMEMORY;

		buf[0] = path_separator;
		buf[1] = '\0';

		info->current_directory = new_current_dir;
		strcat(info->current_directory, dir);	
		strcat(info->current_directory, buf);
	}
	return full_refresh_image(window);
}



static imgtoolerr_t double_click(HWND window)
{
	imgtoolerr_t err;
	struct wimgtool_info *info;
	LVHITTESTINFO htinfo;
	LVITEM item;
	POINTS pt;
	RECT r;
	DWORD pos;
	imgtool_dirent entry;
	int selected_item;

	info = get_wimgtool_info(window);

	memset(&htinfo, 0, sizeof(htinfo));
	pos = GetMessagePos();
	pt = MAKEPOINTS(pos);
	GetWindowRect(info->listview, &r);
	htinfo.pt.x = pt.x - r.left;
	htinfo.pt.y = pt.y - r.top;
	ListView_HitTest(info->listview, &htinfo);
	
	if (htinfo.flags & LVHT_ONITEM)
	{
		memset(&entry, 0, sizeof(entry));

		item.mask = LVIF_PARAM;
		item.iItem = htinfo.iItem;
		ListView_GetItem(info->listview, &item);

		selected_item = item.lParam;
	
		if (selected_item < 0)
		{
			strcpy(entry.filename, "..");
			entry.directory = 1;
		}
		else
		{
			err = img_getdirent(info->image, info->current_directory, selected_item, &entry);
			if (err)
				return err;
		}

		if (entry.directory)
		{
			err = change_directory(window, entry.filename);
			if (err)
				return err;
		}
	}
	return IMGTOOLERR_SUCCESS;
}



static BOOL context_menu(HWND window, LONG x, LONG y)
{
	struct wimgtool_info *info;
	LVHITTESTINFO hittest;
	BOOL rc = FALSE;
	HMENU menu;

	info = get_wimgtool_info(window);

	memset(&hittest, 0, sizeof(hittest));
	hittest.pt.x = x;
	hittest.pt.y = y;
	ScreenToClient(info->listview, &hittest.pt);
	ListView_HitTest(info->listview, &hittest);

	if (hittest.flags & LVHT_ONITEM)
	{
		menu = LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_FILECONTEXT_MENU));
		TrackPopupMenu(GetSubMenu(menu, 0), TPM_LEFTALIGN | TPM_RIGHTBUTTON, x, y, 0, window, NULL);
		DestroyMenu(menu);
		rc = TRUE;
	}
	return rc;
}



static LRESULT CALLBACK wimgtool_wndproc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	struct wimgtool_info *info;
	RECT window_rect;
	RECT status_rect;
	int window_width;
	int window_height;
	int status_height;
	int selected_item;
	NMHDR *notify;
	POINT pt;
	LRESULT lres;
	HWND target_window;
	DWORD style;
	HMENU menu;
	struct imgtool_module_features features;

	info = get_wimgtool_info(window);

	switch(message)
	{
		case WM_CREATE:
			if (wimgtool_create(window, (CREATESTRUCT *) lparam))
				return -1;
			break;

		case WM_DESTROY:
			wimgtool_destroy(window);
			break;

		case WM_SIZE:
			GetClientRect(window, &window_rect);
			GetClientRect(info->statusbar, &status_rect);

			window_width = window_rect.right - window_rect.left;
			window_height = window_rect.bottom - window_rect.top;
			status_height = status_rect.bottom - status_rect.top;

			SetWindowPos(info->listview, NULL, 0, 0, window_width,
				window_height - status_height, SWP_NOMOVE | SWP_NOZORDER);
			SetWindowPos(info->statusbar, NULL, 0, window_height - status_height,
				window_width, status_height, SWP_NOMOVE | SWP_NOZORDER);
			break;

		case WM_INITMENU:
			menu = (HMENU) wparam;
			if (info->image)
				features = img_get_module_features(img_module(info->image));
			else
				memset(&features, 0, sizeof(features));

			selected_item = get_selected_item(window);
			if (selected_item < 0)
			{
				features.supports_reading = 0;
				features.supports_deletefile = 0;
				features.supports_deletedir = 0;
			}

			/* TODO - At some point, we need to enabled ID_IMAGE_DELETE
			 * conditionally based on whether the current selection is a
			 * file or a directory, but for now I'm being lazy */
			EnableMenuItem(menu, ID_IMAGE_INSERT,
				MF_BYCOMMAND | (features.supports_writing ? MF_ENABLED : MF_GRAYED));
			EnableMenuItem(menu, ID_IMAGE_EXTRACT,
				MF_BYCOMMAND | (features.supports_reading ? MF_ENABLED : MF_GRAYED));
			EnableMenuItem(menu, ID_IMAGE_CREATEDIR,
				MF_BYCOMMAND | (features.supports_createdir ? MF_ENABLED : MF_GRAYED));
			EnableMenuItem(menu, ID_IMAGE_DELETE,
				MF_BYCOMMAND | (features.supports_deletefile ? MF_ENABLED : MF_GRAYED));
			break;

		case WM_DROPFILES:
			drop_files(window, (HDROP) wparam);
			break;

		case WM_COMMAND:
			switch(LOWORD(wparam))
			{
				case ID_FILE_NEW:
					menu_new(window);
					break;

				case ID_FILE_OPEN:
					menu_open(window);
					break;

				case ID_FILE_CLOSE:
					PostMessage(window, WM_CLOSE, 0, 0);
					break;

				case ID_IMAGE_INSERT:
					menu_insert(window);
					break;

				case ID_IMAGE_EXTRACT:
					menu_extract(window);
					break;

				case ID_IMAGE_CREATEDIR:
					menu_createdir(window);
					break;

				case ID_IMAGE_DELETE:
					menu_delete(window);
					break;

				case ID_VIEW_ICONS:
					set_listview_style(window, LVS_ICON);
					break;

				case ID_VIEW_LIST:
					set_listview_style(window, LVS_LIST);
					break;

				case ID_VIEW_DETAILS:
					set_listview_style(window, LVS_REPORT);
					break;

				case ID_VIEW_ASSOCIATIONS:
					win_association_dialog(window, library);
					break;
			}
			break;

		case WM_NOTIFY:
			notify = (NMHDR *) lparam;
			switch(notify->code)
			{
				case NM_DBLCLK:
					double_click(window);
					break;

				case LVN_BEGINDRAG:
					pt.x = 8;
					pt.y = 8;

					lres = SendMessage(info->listview, LVM_CREATEDRAGIMAGE,
						(WPARAM) ((NM_LISTVIEW *) lparam)->iItem, (LPARAM) &pt);
					info->dragimage = (HIMAGELIST) lres;

					pt = ((NM_LISTVIEW *) notify)->ptAction;
					ClientToScreen(info->listview, &pt);

					ImageList_BeginDrag(info->dragimage, 0, 0, 0);
					ImageList_DragEnter(GetDesktopWindow(), pt.x, pt.y);
					SetCapture(window);
					info->dragpt = pt;
					break;
			}
			break;

		case WM_MOUSEMOVE:
			if (info->dragimage)
			{
				pt.x = LOWORD(lparam);
				pt.y = HIWORD(lparam);
				ClientToScreen(window, &pt);
				info->dragpt = pt;

				ImageList_DragMove(pt.x, pt.y);
			}
			break;

		case WM_LBUTTONUP:
			if (info->dragimage)
			{
				target_window = WindowFromPoint(info->dragpt);

				ImageList_DragLeave(info->listview);
				ImageList_EndDrag();
				ImageList_Destroy(info->dragimage);
				ReleaseCapture();
				info->dragimage = NULL;
				info->dragpt.x = 0;
				info->dragpt.y = 0;

				style = GetWindowLong(target_window, GWL_EXSTYLE);
				if (style & WS_EX_ACCEPTFILES)
				{
				}
			}
			break;

		case WM_CONTEXTMENU:
			context_menu(window, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
			break;
	}

	return DefWindowProc(window, message, wparam, lparam);
}



BOOL wimgtool_registerclass(void)
{
	WNDCLASS wimgtool_wndclass;

	memset(&wimgtool_wndclass, 0, sizeof(wimgtool_wndclass));
	wimgtool_wndclass.lpfnWndProc = wimgtool_wndproc;
	wimgtool_wndclass.lpszClassName = wimgtool_class;
	wimgtool_wndclass.lpszMenuName = MAKEINTRESOURCE(IDR_WIMGTOOL_MENU);
	wimgtool_wndclass.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_IMGTOOL));
	return RegisterClass(&wimgtool_wndclass);
}



