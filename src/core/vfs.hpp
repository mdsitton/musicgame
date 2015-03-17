#pragma once
#include "vfs/ttvfs.h"
#include "vfs/ttvfs_zip.h"

//
// To add archive to VFS:
// 	VFS.AddArchive( "test.zip" );
//
// To mount archive to root of VFS:
// 	VFS.Mount( "test.zip", "" );
//
// To mount folder to root of VFS:
// 	VFS.Mount( "dataDir", "" );
//
// To use file that is in VFS:
//	ttvfs::File *vf = VFS.GetFile( "data.txt" );
//	vf->open( "r" );
//	char buf[513];
//	size_t bytes = vf->read( buf, 512 );
//	puts( buf );
//	vf->close();
//
//
// Notes:
// - Archives MUST be added before being mounted.
// - The second arguement to VFS.Mount() is the alias you want to use when
//   accessing the folder/archive. Setting it to "" will add it's contents
//   to the root of the VFS, effectively overwriting same-named files with
//   the new ones.
// - Any directory or archive in the VFS can also be re-mounted.
//
 
extern ttvfs::Root VFS;