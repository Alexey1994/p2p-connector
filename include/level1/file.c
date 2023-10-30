#ifndef FILE_INCLUDED
#define FILE_INCLUDED


#include <types.c>
#include <memory.c>

/*
typedef Byte* File;

Boolean  create_directory(Byte* path);
Boolean  delete_directory(Byte* path);

void     find_files (
	Byte* search_path,
	void(*on_find)(
		Byte*    context,
		Byte*    name,
		Boolean  is_directory,
		Number32 size_low,
		Number32 size_hight,
		Number   index
	),
	Byte* context
);

File     create_file                  (Byte* path);
File     open_file                    (Byte* path);
Number   read_bytes_from_file         (File file, Byte* bytes, Number32 size_of_bytes);
Number   write_bytes_in_file          (File file, Byte* bytes, Number32 size_of_bytes);
Number   get_file_size                (File file);
void     offset_current_file_position (File file, Number32 position);
void     set_file_position            (File file, Number32 position);
Number   get_file_position            (File file);
void     set_file_end_position        (File file);
void     close_file                   (File file);
Boolean  delete_file                  (Byte* path);
*/

#ifdef __WIN32__

#include <WinAPI/kernel32.c>
#include <string.c>


Boolean create_directory(Byte* path)
{
	Number32  status;
	Number16* unicode_path;

	unicode_path = convert_utf8_to_unicode(path);
	status = CreateDirectoryW(unicode_path, 0);
	free_memory(unicode_path);

	return status;
}


Boolean delete_directory(Byte* path)
{
	Number32  status;
	Number16* unicode_path;

	unicode_path = convert_utf8_to_unicode(path);
	status = RemoveDirectoryW(unicode_path);
	free_memory(unicode_path);

	return status;
}


Number get_current_directory(Byte* path, Number size_of_path)
{
	Number16* unicode_path;
	Number    unicode_path_size;
	Byte*     utf8_path;
	Number    i;
	
	unicode_path = allocate_memory(32768 * sizeof(Number16));
	unicode_path_size = GetCurrentDirectoryW(32768, unicode_path);
	
	utf8_path = convert_unicode_to_utf8(unicode_path);
	free_memory(unicode_path);
	for(i = 0; utf8_path[i] && i < size_of_path - 1; ++i) {
		path[i] = utf8_path[i];
	}
	free_memory(utf8_path);
	
	path[i] = '\0';
	
	return i;
}


Boolean set_current_directory(Byte* path)
{
	Boolean   state;
	Number16* unicode_path;
	
	unicode_path = convert_utf8_to_unicode(path);
	state = SetCurrentDirectoryW(unicode_path);
	free_memory(unicode_path);
	
	return state;
}


Boolean open_directory(Byte* path)
{
	Boolean state;
	Byte*   current_path;
	Number  current_directory_size;
	Number  i;
	
	current_path = allocate_memory(32768);
	current_directory_size = get_current_directory(current_path, 32768);
	
	current_path[current_directory_size] = '\\';
	++current_directory_size;
	for(i = 0; path[i] && current_directory_size < 32768 - 1; ++i) {
		current_path[current_directory_size] = path[i];
		++current_directory_size;
	}
	current_path[current_directory_size] = '\0';
	
	state = set_current_directory(current_path);
	free_memory(current_path);
	
	return state;
}


/*
Boolean file_exist(Byte* path)
{
	File      file;
	File_Data file_data;

	file = OpenFile(path, &file_data, OPEN_FILE_READ);
	CloseHandle(file);

	return !file_data.error_code;
}*/


File create_file(Byte* path)
{
	File     file;
	Number32 bytes_writed;

	//if(file_exist(path))
	//	goto error;

	Number16* unicode_path;
	unicode_path = convert_utf8_to_unicode(path);
	file = CreateFileW(unicode_path, GENERIC_WRITE, DISABLE_ALL_FILE_OPERATION, 0, CREATE_ALWAYS, NORMAL_FILE_ATTRIBUTE, 0);
	free_memory(unicode_path);

	if(file == -1)
		return 0;

	return file;
}


Boolean create_hard_link(Byte* link_name, Byte* existing_file_name)
{
	Byte*   unicode_link_name;
	Byte*   unicode_existing_file_name;
	Boolean result;
	
	unicode_link_name = convert_utf8_to_unicode(link_name);
	unicode_existing_file_name = convert_utf8_to_unicode(existing_file_name);
	result = CreateHardLinkW(unicode_link_name, unicode_existing_file_name, 0);
	free_memory(unicode_link_name);
	free_memory(unicode_existing_file_name);
	
	return result;
}


Boolean create_symbolic_link(Byte* link_name, Byte* existing_file_name, Boolean is_directory)
{
	Byte*   unicode_link_name;
	Byte*   unicode_existing_file_name;
	Boolean result;
	
	unicode_link_name = convert_utf8_to_unicode(link_name);
	unicode_existing_file_name = convert_utf8_to_unicode(existing_file_name);
	result = CreateSymbolicLinkW(unicode_link_name, unicode_existing_file_name, is_directory);
	free_memory(unicode_link_name);
	free_memory(unicode_existing_file_name);
	
	return result;
}


void find_files(
	Byte* search_path,
	void(*on_find)(
		Byte*    context,
		Byte*    name,
		Boolean  is_directory,
		Number32 size_low,
		Number32 size_hight,
		Number   index
	),
	Byte* context
)
{
	Find_File_Information file_information;
	Byte*                 file_finder;
	Number16*             unicode_search_path;
	Byte*                 utf8_name;
	Number                index;

	unicode_search_path = convert_utf8_to_unicode(search_path);
	file_finder = FindFirstFileW(unicode_search_path, &file_information);
	free_memory(unicode_search_path);

	if(file_finder) {
		index = 0;
		utf8_name = convert_unicode_to_utf8(file_information.file_name);
		on_find(context, utf8_name, file_information.attributes & DIRECTORY_FILE_ATTRIBUTE, file_information.file_size_low, file_information.file_size_high, index);
		free_memory(utf8_name);
		++index;

		while(FindNextFileW(file_finder, &file_information)) {
			utf8_name = convert_unicode_to_utf8(file_information.file_name);
			on_find(context, utf8_name, file_information.attributes & DIRECTORY_FILE_ATTRIBUTE, file_information.file_size_low, file_information.file_size_high, index);
			free_memory(utf8_name);
			++index;
		}

		FindClose(file_finder);
	}
}


typedef struct {
	Boolean  is_folder;
	Boolean  is_link;
	Number32 link_type;
	//Boolean  is_symlink;
	//Boolean  is_hardlink;
	Number64 creation_time;
	Number64 last_access_time;
	Number64 last_write_time;
	Byte     name[260];
}
File_Info;


Byte* enum_next_file_link(Byte** enumerator, Byte* file_name)
{
	Number32  unicode_link_name_size = 32768;
	Number16* unicode_link_name = allocate_memory(unicode_link_name_size * sizeof(Number16));
	
	if(*enumerator) {
		if(!FindNextFileNameW(*enumerator, &unicode_link_name_size, unicode_link_name)) {
			FindClose(*enumerator);
			*enumerator = 0;
			free_memory(unicode_link_name);
			return 0;
		}
	}
	else {
		Number16* unicode_file_name = convert_utf8_to_unicode(file_name);
		*enumerator = FindFirstFileNameW(unicode_file_name, 0, &unicode_link_name_size, unicode_link_name);
		free_memory(unicode_file_name);
		
		if(!*enumerator) {
			free_memory(unicode_link_name);
			return 0;
		}
	}
	
	Byte* utf8_link_name = convert_unicode_to_utf8(unicode_link_name);
	free_memory(unicode_link_name);
	
	return utf8_link_name;
}


Boolean enum_next_file(Byte** enumerator, File_Info* file_info)
{
	Find_File_Information winapi_file_info;
	Byte*                 utf8_name;
	
	if(*enumerator) {
		if(!FindNextFileW(*enumerator, &winapi_file_info)) {
			FindClose(*enumerator);
			*enumerator = 0;
			return 0;
		}
	}
	else {
		*enumerator = FindFirstFileW(L"./*", &winapi_file_info);
		
		if(!*enumerator) {
			return 0;
		}
	}
	
	utf8_name = convert_unicode_to_utf8(winapi_file_info.file_name);
	
	file_info->is_folder = winapi_file_info.attributes & DIRECTORY_FILE_ATTRIBUTE;
	file_info->is_link = winapi_file_info.attributes & REPARSE_POINT_FILE_ATTRIBUTE;
	
	if(file_info->is_link) {
		file_info->link_type = winapi_file_info.reserved1 & 0x0000FFFF;
	}
	else if(!file_info->is_folder) {
		//test for hard link
		
		Byte* link_enumerator;
		Number32  unicode_link_name_size = 32768;
		Number16* unicode_link_name = allocate_memory(unicode_link_name_size * sizeof(Number16));
		
		link_enumerator = FindFirstFileNameW(winapi_file_info.file_name, 0, &unicode_link_name_size, unicode_link_name);
		
		Byte* current_dir = allocate_memory(32768);
		get_current_directory(current_dir, 32768);
		
		Number path_index;
		
		for(path_index = 0; current_dir[path_index] && current_dir[path_index] != ':'; ++path_index) {
			
		}
		if(current_dir[path_index] == ':') {
			++path_index;
		}
		
		Byte* full_path = allocate_memory(32768);
		print_in_string(full_path, "%s\\%s", current_dir + path_index, utf8_name);
		free_memory(current_dir);
		
		Byte* utf8_link_name = convert_unicode_to_utf8(unicode_link_name);
		free_memory(unicode_link_name);
		
		if(compare_null_terminated_bytes(full_path, utf8_link_name)) {
			file_info->is_link = 1;
			file_info->link_type = -1;
		}
		
		free_memory(full_path);
		free_memory(utf8_link_name);
	}
	file_info->creation_time = winapi_file_info.creation_time;
	file_info->last_access_time = winapi_file_info.last_access_time;
	file_info->last_write_time = winapi_file_info.last_write_time;
	
	Number i;
	for(i = 0; utf8_name[i] && i < sizeof(file_info->name) - 1; ++i) {
		file_info->name[i] = utf8_name[i];
	}
	file_info->name[i] = '\0';
	
	free_memory(utf8_name);
	
	return 1;
}


Number32 get_file_attributes(Byte* path)
{
	Number16* unicode_path = convert_utf8_to_unicode(path);
	Number32  attributes = GetFileAttributesW(unicode_path);
	free_memory(unicode_path);
	return attributes;
	
	//File_Information info;
	
	//GetFileInformationByHandle(file, &info);
	
	//return info.attributes;
}


Boolean delete_file(Byte* path)
{
	Number32  status;
	Number16* unicode_path;
	
	unicode_path = convert_utf8_to_unicode(path);
	DeleteFileW(unicode_path);
	free_memory(unicode_path);

	return status;
}


File open_file(Byte* path)
{
	File      file;
	Number16* unicode_path;

	unicode_path = convert_utf8_to_unicode(path);
	file = CreateFileW(unicode_path, GENERIC_READ | GENERIC_WRITE, /*DISABLE_ALL_FILE_OPERATION*/ENABLE_READ_FILE_OPERATION | ENABLE_WRITE_FILE_OPERATION, 0, OPEN_EXISTING, 0, 0);//OpenFile(path, &file_data, OPEN_FILE_READ_AND_WRITE);
	free_memory(unicode_path);

	if(file == -1)
		return 0;

	return file;
}


File open_file_for_read(Byte* path)
{
	File      file;
	Number16* unicode_path;

	unicode_path = convert_utf8_to_unicode(path);
	file = CreateFileW(unicode_path, GENERIC_READ, ENABLE_READ_FILE_OPERATION, 0, OPEN_EXISTING, 0, 0);
	free_memory(unicode_path);

	if(file == -1)
		return 0;

	return file;
}


File open_file_for_get_path(Byte* path)
{
	File      file;
	Number16* unicode_path;

	unicode_path = convert_utf8_to_unicode(path);
	//file = CreateFileW(unicode_path, GENERIC_READ | GENERIC_WRITE, ENABLE_READ_FILE_OPERATION | ENABLE_WRITE_FILE_OPERATION, 0, OPEN_EXISTING, BACKUP_SEMANTICS_FILE_ATTRIBUTE, 0);
	file = CreateFileW(unicode_path, 0, 0, 0, OPEN_EXISTING, BACKUP_SEMANTICS_FILE_ATTRIBUTE, 0);
	free_memory(unicode_path);

	if(file == -1)
		return 0;

	return file;
}


Number read_bytes_from_file(File file, Byte* bytes, Number32 size_of_bytes)
{
	Number bytes_readed;

	//SetFilePointer(file, position, ((Byte*)&position) + sizeof(Number32), BEGIN_FILE_POSITION);

	bytes_readed = 0;

	if(!ReadFile(file, bytes, size_of_bytes, &bytes_readed, 0)/* || bytes_readed != size_of_bytes*/)
		goto error;

	return bytes_readed;

	error: {
		//print("%d readed\n", bytes_readed);
		return 0;
	}
}


Number write_bytes_in_file(File file, Byte* bytes, Number32 size_of_bytes)
{
	Number bytes_writed;

	//SetFilePointer(file, position, ((Byte*)&position) + sizeof(Number32), BEGIN_FILE_POSITION);

	bytes_writed = 0;

	if(!WriteFile(file, bytes, size_of_bytes, &bytes_writed, 0) || bytes_writed != size_of_bytes)
		goto error;

	return bytes_writed;

	error: {
		//print("%d writed\n", bytes_writed);
		return 0;
	}
}


Number get_file_size(File file)
{
	Number64 file_size;
	Number32 bytes_returned;

	if(!GetFileSizeEx(file, &file_size)) {
		if(!DeviceIoControl(file, IOCTL_DISK_GET_LENGTH_INFO, 0, 0, &file_size, sizeof(file_size), &bytes_returned, 0))
			return 0;
	}

	return file_size;
}


void offset_current_file_position(File file, Number32 position)
{
	SetFilePointer(file, position, 0, CURRENT_FILE_POSITION);
}


void set_file_position(File file, Number64 position)
{
	Number32* position_tails;
	
	position_tails = &position;
	SetFilePointer(file, position_tails[0], position_tails + 1, BEGIN_FILE_POSITION);
}


Number get_file_position(File file)
{
	return SetFilePointer(file, 0, 0, CURRENT_FILE_POSITION);
}


void set_file_end_position(File file)
{
	SetFilePointer(file, 0, 0, END_FILE_POSITION);
}


void close_file (File file)
{
	CloseHandle(file);
}

/*
void wait_for_directory_changes()
{
	File              folder;
	Number32          bytes_returned;
	File_Notification notifications[1024];

	folder = CreateFileW(
		L"./",
		FILE_LIST_DIRECTORY,
		ENABLE_READ_FILE_OPERATION | ENABLE_WRITE_FILE_OPERATION | ENABLE_DELETE_FILE_OPERATION,
		0,
		OPEN_EXISTING,
		BACKUP_SEMANTICS_FILE_ATTRIBUTE,
		0
	);

	ReadDirectoryChangesW(
		folder,
		notifications,
		sizeof(notifications),
		1,
		FILE_NOTIFY_CHANGE_LAST_WRITE,
		&bytes_returned,
		0,
		0
	);

	CloseHandle(folder);
}*/

typedef struct {
	File              folder;
	Number32          bytes_returned;
	File_Notification notifications[1024];

	void(*on_changes)(Byte* arguments);
	Byte* on_changes_arguments;
}
Directory_Changes;

static stdcall void notify_directory_changes(Number32 error_code, Number32 bytes_transfered, Directory_Changes* changes);

static Boolean read_next_directory_changes(Directory_Changes* changes)
{
	Number status;

	changes->folder = CreateFileW(
		L"./",
		FILE_LIST_DIRECTORY,
		ENABLE_READ_FILE_OPERATION | ENABLE_WRITE_FILE_OPERATION | ENABLE_DELETE_FILE_OPERATION,
		0,
		OPEN_EXISTING,
		BACKUP_SEMANTICS_FILE_ATTRIBUTE | OVERLAPPED_FILE_ATTRIBUTE,
		0
	);

	status = ReadDirectoryChangesW(
		changes->folder,
		changes->notifications,
		sizeof(changes->notifications),
		1,
		FILE_NOTIFY_CHANGE_LAST_WRITE,
		&changes->bytes_returned,
		changes,
		&notify_directory_changes
	);

	CloseHandle(changes->folder);

	return !!status;
}

static stdcall void notify_directory_changes(Number32 error_code, Number32 bytes_transfered, Directory_Changes* changes)
{
	read_next_directory_changes(changes);

	changes->on_changes(changes->on_changes_arguments);

	//print(
	//    "notify: error code = ", _Number, error_code,
	//    ", bytes transfered = ", _Number, bytes_transfered,
	//    ", changes = ", _Number, changes,
	//    "\n"
	//)
}

Boolean read_directory_changes(Directory_Changes* changes, void(*on_changes)(Byte* arguments), Byte* arguments)
{
	changes->on_changes = on_changes;
	changes->on_changes_arguments = arguments;
	return read_next_directory_changes(changes);
}


void print_character_in_file(File file, Number32 character)
{
	write_bytes_in_file(file, &character, 1);
}

void print_in_file(File file, Byte* parameters, ...)
{
	print_in_source(file, &print_character_in_file, parameters, &parameters + 1);
}


#endif//__WIN32__

/*
#include <reader.c>


void initialize_file_reader(Reader* reader, Byte* file_path)
{
	File file;

	file = open_file(file_path);

	initialize_reader(reader, file, &read_bytes_from_file);
	reader->close_source = &close_file;
}


void initialize_file_reader_from_file (Reader* reader, File file)
{
	initialize_reader(reader, file, &read_bytes_from_file);
}


void initialize_file_writer (Writer* writer, Byte* file_path)
{
	File file;

	file = create_file(file_path);

	initialize_writer(writer, file, &write_bytes_in_file);
	writer->close_source = &close_file;
}*/


#endif//FILE_INCLUDED

/*
void main()
{	
	File file;

	Byte     data[256];
	Number32 bytes;

	file = open_file("привет.txt");
	
	if(!file)
		file = create_file("привет.txt");

	write_bytes_in_file(file, "привет", sizeof("привет") - 1);
	close_file(file);

	file = open_file("привет.txt");
	read_bytes_from_file(file, data, sizeof("привет") - 1);
	data[sizeof("привет") - 1] = '\0';
	print(
		data, " ",
		_Number, (Number32)get_file_size(file), " bytes"
	);
	close_file(file);
}*/

/*
Number16 result[20480];

void main()
{
	Number i;
	Number size;

	size = QueryDosDeviceW(0, result, 20480);

	i = 0;
	while(result[i]) {
		for(; result[i]; ++i)
		printf("%c", result[i]);
		printf("\n");
		++i;
	}

	//Byte name[256];
	//GetVolumeInformationA("C:/", name, 256, 0, 0, 0, 0, 0);
	//printf(name);

	system("pause");
}*/
