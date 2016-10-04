/**
 * idevicefilesharing --  simple application file sharing manager 
 *                                           for iPhone/iPod/iPad
 *
 * Copyright (C) 2016 Aymen Ibrahim <aymen_ibrahim@hotmail.com>
 *
 * Licensed under the GNU General Public License Version 2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more profile.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 
 * USA
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <plist/plist.h>
#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/lockdown.h>
#include <libimobiledevice/installation_proxy.h>
#include <libimobiledevice/afc.h>
#include <libimobiledevice/house_arrest.h>

enum {
	OP_LIST_FILE_SHARING_APPS,
	OP_LIST_FILES,
	OP_PUT_FILE,
	OP_REMOVE_FILE,
	NUM_OPS
};

typedef void (*afc_file_upload_status_cb_t) (const char *filename, int uploaded , int total_size);

static void print_usage(int argc, const char **argv);
static void list_file_sharing_apps(idevice_t device, lockdownd_client_t client);

// afc
static int afc_list_files(afc_client_t afc);
static int afc_upload_file(afc_client_t afc, const char* filename, const char* dstfn , afc_file_upload_status_cb_t callback);
static int afc_delete_path(afc_client_t afc_client, const char* filename);

static int create_afc_client(idevice_t device, lockdownd_client_t client, const char* bundle_id, afc_client_t* afc_client, house_arrest_client_t* house_arrest_client2);

// utils
void extract_filename(const char* path, char** filename);
static int parse_filenames(const char** argv ,const char*** filepaths,int start, int argc);

// callbacks
static void upload_callback(const char* filename,  int uploaded , int total_size);


static void print_plist(plist_t plist)
{
    char * test = NULL; uint length = 0;
    plist_to_xml(plist , &test , &length);
    printf("%.*s", length, test);
    free(test);
}

int main(int argc, char const *argv[])
{
	lockdownd_client_t client = NULL;
	lockdownd_error_t ldret = LOCKDOWN_E_UNKNOWN_ERROR;
	lockdownd_service_descriptor_t service = NULL;
	idevice_t device = NULL;
	idevice_error_t ret = IDEVICE_E_UNKNOWN_ERROR;

	int op = -1;

	const char* udid = NULL;
	const char* bundle_id = NULL;
	const char** filepaths = NULL;
	int filepaths_count = 0;

	/* parse cmdline args */
	int i;
	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-u") || !strcmp(argv[i], "--udid")) {
			i++;
			if (!argv[i] || (strlen(argv[i]) != 40)) {
				print_usage(argc, argv);
				return 0;
			}
			udid = argv[i];
			continue;
		}

		else if (!strcmp(argv[i], "list_apps")) {
			i++;
			op = OP_LIST_FILE_SHARING_APPS;
			
		}
		else if (!strcmp(argv[i], "list")) {
			i++;	
			if (!argv[i] || (strlen(argv[i]) < 1)) {
				print_usage(argc, argv);
				return 0;
			}
			bundle_id = argv[i];
			op = OP_LIST_FILES;
			continue;
		}
		else if (!strcmp(argv[i], "delete")) {
			i++;	
			if (!argv[i] || (strlen(argv[i]) < 1)) {
				print_usage(argc, argv);
				return 0;
			}
			bundle_id = argv[i];

			i++;	
			if (!argv[i] || (strlen(argv[i]) < 1)) {
				print_usage(argc, argv);
				return 0;
			}

			int c = parse_filenames(argv , &filepaths,i, argc);
			i+= c;
			filepaths_count = c;

			op = OP_REMOVE_FILE;
			continue;
		}
		else if (!strcmp(argv[i], "put")) {
			i++;	
			if (!argv[i] || (strlen(argv[i]) < 1)) {
				print_usage(argc, argv);
				return 0;
			}
			bundle_id = argv[i];
			i++;	
			if (!argv[i] || (strlen(argv[i]) < 1)) {
				print_usage(argc, argv);
				return 0;
			}

			int c = parse_filenames(argv , &filepaths,i, argc);
			i+= c;
			filepaths_count = c;

			op = OP_PUT_FILE;
			continue;
		}
		else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
			print_usage(argc, argv);
			return 0;
		}
		else {
			print_usage(argc, argv);
			return 0;
		}
	}

	if ((op == -1) || (op >= NUM_OPS)) {
	print_usage(argc, argv);
	return 0;
	}

	ret = idevice_new(&device, udid);

	if (ret != IDEVICE_E_SUCCESS) {
		if (udid) {
			printf("No device found with udid %s, is it plugged in?\n", udid);
		} else {
			printf("No device found, is it plugged in?\n");
		}
		return -1;
	}

	if (LOCKDOWN_E_SUCCESS != (ldret = lockdownd_client_new_with_handshake(device, &client, "idevicefilesharing"))) {
		fprintf(stderr, "ERROR: Could not connect to lockdownd, error code %d\n", ldret);
		idevice_free(device);
		return -1;
	}

	switch(op)
	{
		case OP_LIST_FILE_SHARING_APPS:
			list_file_sharing_apps(device, client);
			break;

		case OP_LIST_FILES:
			{
				afc_client_t afc_client = NULL;
				house_arrest_client_t house_arrest_client = NULL;
				create_afc_client(device, client,bundle_id,&afc_client, &house_arrest_client);
				if (afc_client)
				{
					afc_list_files(afc_client);
				}
				if(house_arrest_client) house_arrest_client_free(house_arrest_client);

			}
			break;
		case OP_REMOVE_FILE:
			{
				afc_client_t afc_client = NULL;
				house_arrest_client_t house_arrest_client = NULL;
				create_afc_client(device, client,bundle_id,&afc_client, &house_arrest_client);

				if (afc_client && filepaths)
				{
					for (int i = 0; i < filepaths_count; ++i)
					{
						const char* filepath = filepaths[i];
						printf("deleting %s\n", filepath);
						afc_delete_path(afc_client, filepath);
					}
				}
				if(house_arrest_client) house_arrest_client_free(house_arrest_client);
			}
			break;
		case OP_PUT_FILE:
			{
				afc_client_t afc_client = NULL;
				house_arrest_client_t house_arrest_client = NULL;
				create_afc_client(device, client,bundle_id,&afc_client, &house_arrest_client);

				if (afc_client && filepaths)
				{
					for (int i = 0; i < filepaths_count; ++i)
					{
						const char* filepath = filepaths[i];
						char* temp;
						extract_filename(filepath , &temp);
						char *dstfn = (char*)malloc(strlen("/Documents/") + strlen(temp) + 1);
						strcpy(dstfn, "/Documents/");
						strcat(dstfn, temp);
						printf("putting %s\n", temp);
						afc_upload_file(afc_client, filepath , dstfn, &upload_callback);
					}
					
				}
				if(house_arrest_client) house_arrest_client_free(house_arrest_client);
			}
			break;
		default:
			break;
	}

	if (filepaths) free(filepaths);
	return 0;
}

static void print_usage(int argc, const char **argv)
{
	char *name = NULL;
	name = strrchr(argv[0], '/');
	printf("Usage: %s [OPTIONS] COMMAND\n", (name ? name + 1: argv[0]));
	printf("Manage App file sharing for idevice.\n\n");
	printf(" Where COMMAND is one of:\n");
	printf("  list_apps            \tlist installed apps with file sharing enabled.\n");
	printf("       list APP_ID     \tlist of files in the apps file sharing directory.\n");
	printf("        put APP_ID FILE [..] copy files into the file sharing folder of the app\n");
	printf("     delete APP_ID FILE [..] delete files from the app file sharing folder\n");
	printf("\n");	
	printf(" The following OPTIONS are accepted:\n");
	printf("  -u, --udid UDID  target specific device by its 40-digit device UDID\n");
	printf("  -h, --help       prints usage information\n");
	printf("\n");
}

static void list_file_sharing_apps(idevice_t device, lockdownd_client_t client)
{
	instproxy_client_t ipc = NULL;
	instproxy_error_t err;
	lockdownd_service_descriptor_t service = NULL;

	if ((lockdownd_start_service(client, "com.apple.mobile.installation_proxy", &service) != LOCKDOWN_E_SUCCESS) || !service) 
	{
		fprintf(stderr, "Could not start com.apple.mobile.installation_proxy!\n");
	}

	err = instproxy_client_new(device, service, &ipc);
	if (service)  lockdownd_service_descriptor_free(service);
	service = NULL;

	if(err!= INSTPROXY_E_SUCCESS)
	{
		fprintf(stderr, "installation proxy returned with error code %d\n", err);
	}


	plist_t client_opts = instproxy_client_options_new();
	instproxy_client_options_add(client_opts, "ApplicationType", "User", NULL);
	plist_t apps = NULL;

	err = instproxy_browse(ipc, client_opts, &apps);
	instproxy_client_options_free(client_opts);

	uint32_t i = 0;
	for (i = 0; i < plist_array_get_size(apps); i++) 
	{
		plist_t app = plist_array_get_item(apps, i);
		plist_t sharing_enabled = plist_dict_get_item(app, "UIFileSharingEnabled");
		if(sharing_enabled)
		{	
			uint8_t enabled = 0;
			plist_get_bool_val(sharing_enabled, &enabled);
			if(enabled)
			{
				plist_t pbundle_id = plist_dict_get_item(app, "CFBundleIdentifier");
				plist_t pdisplay_name = plist_dict_get_item(app, "CFBundleDisplayName");

				char *bundle_id = NULL;
				char *display_name = NULL;
				if (pbundle_id)  
					plist_get_string_val(pbundle_id, &bundle_id);
				else
					bundle_id = "";

				if (pdisplay_name) 
					plist_get_string_val(pdisplay_name, &display_name);
				else
					display_name = "";

				printf("%s - %s\n", display_name, bundle_id );
			}	
		}
	}

	if (ipc) instproxy_client_free(ipc);
	if (apps) plist_free(apps);
}

static int create_afc_client(idevice_t device, lockdownd_client_t client, const char* bundle_id, afc_client_t* afc_client, house_arrest_client_t* house_arrest_client2)
{
	if(!bundle_id)
	{
		fprintf(stderr, "Error: app bundle id is required\n");
		return -2;
	}

	house_arrest_client_t house_arrest_client = NULL;
	house_arrest_error_t err;
	lockdownd_service_descriptor_t service = NULL;

	if ((lockdownd_start_service(client, "com.apple.mobile.house_arrest" , &service) != LOCKDOWN_E_SUCCESS) || !service) 
	{
		fprintf(stderr, "Could not start com.apple.mobile.house_arrest!\n");
	}

	err = house_arrest_client_new(device, service, &house_arrest_client);
	if (service)  lockdownd_service_descriptor_free(service);
	service = NULL;
	if(err!= HOUSE_ARREST_E_SUCCESS)
	{
		fprintf(stderr, "house_arrest_client_new returned with error code %d\n", err);
		return -1;
	}

	err = house_arrest_send_command(house_arrest_client, "VendDocuments" , bundle_id);
	if (err == HOUSE_ARREST_E_SUCCESS)
	{
		plist_t p = NULL;
		house_arrest_get_result(house_arrest_client, &p);
	}
	afc_client_t _afc = NULL;
	afc_error_t afc_err = afc_client_new_from_house_arrest_client(house_arrest_client, &_afc);

	if(_afc)
		*afc_client = _afc;

	*house_arrest_client2 = house_arrest_client;
	return 0;
}

static int afc_list_files(afc_client_t afc_client)
{
	char** dir_infos = NULL;
	afc_error_t err =   afc_read_directory(afc_client , "/Documents", &dir_infos);
	if (dir_infos)
		for (int i = 0; NULL != dir_infos[i]; i++)
		{
			if (!strcmp(dir_infos[i], ".") || !strcmp(dir_infos[i], "..")) continue;
			printf("%s\n", dir_infos[i]);
		}
	afc_dictionary_free(dir_infos);

	return 0;
}

static int afc_delete_path(afc_client_t afc_client, const char* filename)
{
	char *fname = (char*)malloc(strlen("/Documents/") + strlen(filename) + 1);
	strcpy(fname, "/Documents/");
	strcat(fname, filename);
	return afc_remove_path(afc_client, fname);
}

static int afc_upload_file(afc_client_t afc, const char* filename, const char* dstfn , afc_file_upload_status_cb_t callback)
{
	FILE *f = NULL;
	uint64_t af = 0;
	char buf[256 * 1024]; // 256 KB //1048576 // 1 MB
	uint32_t file_size = 0;

	f = fopen(filename, "rb");
	if (!f) 
	{
		// fprintf(stderr, "fopen: %s: %s\n", appid, strerror(errno));
		return -1;
	}

	fseek(f, 0L, SEEK_END);
	file_size = ftell(f);
	// go back
	fseek(f, 0L, SEEK_SET);

	if ((afc_file_open(afc, dstfn, AFC_FOPEN_WRONLY, &af) != AFC_E_SUCCESS) || !af) 
	{
		fclose(f);
		fprintf(stderr, "afc_file_open on '%s' failed!\n", dstfn);
		return -1;
	}

	size_t amount = 0;
	uint32_t grand_total = 0;
	do {
		amount = fread(buf, 1, sizeof(buf), f);
		if (amount > 0)
		 {
			uint32_t written, total = 0;
			while (total < amount) 
			{
				written = 0;
				afc_error_t aerr = afc_file_write(afc, af, buf, amount, &written);
				if (aerr != AFC_E_SUCCESS) 
				{
					fprintf(stderr, "AFC Write error: %d\n", aerr);
					break;
				}
				total += written;
				grand_total += written;
				if(callback) callback(dstfn, grand_total, file_size);
			}
			if (total != amount) 
			{
				fprintf(stderr, "Error: wrote only %d of %zu\n", total, amount);
				afc_file_close(afc, af);
				fclose(f);
				return -1;
			}
		}
	} while (amount > 0);

	afc_file_close(afc, af);
	fclose(f);

	return 0;
}

void extract_filename(const char* path, char** filename)
{
	int found = 0;
	int i = 0; int len = strlen(path);
	char* name = malloc(len * sizeof(char*));
	strcpy(name, path);
	for (i = 0;  i < len ; i++)
	{
		if (name[i] == '\\' || name[i] == '/')
		{
			*filename = &name[i+1];
			found = 1;
		}
	}

	if(!found)
	{
		*filename = name;
	}		
}

static int parse_filenames(const char** argv ,const char*** filepaths,int start, int argc)
{
	if (argc <= start) return 0;
	char** temp = (char**) malloc((argc - start) * sizeof(char*));
	int i = 0;
	for (i = start; i < argc; i++)
	{
			temp[i - start] = (char*)argv[i]; 
	}

	*filepaths = (const char**)temp;
	return  argc - start;
}

static void upload_callback(const char* filename,  int uploaded , int total_size)
{
	// printf("\r%.2f%% uploaded", 100* (uploaded/(double)total_size));
	int j  = 50 * (uploaded/(double)total_size);
	printf("\r\t[");
	for (int i = 0; i < 50; ++i)
		(i <= j) ? printf("=") : printf(" ");
	printf("] %.2f%%", 100* (uploaded/(double)total_size));
	fflush(stdout);

	if (uploaded == total_size) printf("\n");
}

