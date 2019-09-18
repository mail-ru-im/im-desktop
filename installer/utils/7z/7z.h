#pragma once

#pragma warning(disable:4786)

#include "7z_types.h"
#include "7z_header.h"

#define COMM_EXTRACT			1
#define COMM_TEST				2
#define COMM_EXTRACTTOMEMORY	3	


#define kCrcPoly 0xEDB88320
#define kMethodIDSize 15

#ifndef UInt32
#ifdef _LZMA_UINT32_IS_ULONG
#define UInt32 unsigned long
#else
#define UInt32 unsigned int
#endif
#endif

#ifndef SizeT
#ifdef _LZMA_SYSTEM_SIZE_T
#include <stddef.h>
#define SizeT size_t
#else
#define SizeT UInt32
#endif
#endif

#ifdef _LZMA_PROB32
#define CProb UInt32
#else
#define CProb unsigned short
#endif

#define LZMA_RESULT_OK 0
#define LZMA_RESULT_DATA_ERROR 1

#define LzmaGetNumProbs(Properties) (LZMA_BASE_SIZE + (LZMA_LIT_SIZE << ((Properties)->lc + (Properties)->lp))) 

#define LZMA_BASE_SIZE 1846
#define LZMA_LIT_SIZE 768

#define LZMA_PROPERTIES_SIZE 5
 
typedef struct _CMethodID
{
  Byte ID[kMethodIDSize];
  Byte IDSize;
} CMethodID;

typedef struct _ISzAlloc
{
  void *(*Alloc)(size_t size);
  void (*Free)(void *address); /* address can be 0 */
} ISzAlloc; 


typedef struct _CSzByteBuffer
{    
	size_t Capacity;
	Byte *Items;
}CSzByteBuffer; 

typedef struct _CCoderInfo
{
  UInt32 NumInStreams;
  UInt32 NumOutStreams;
  CMethodID MethodID;
  CSzByteBuffer Properties;
}CCoderInfo;

typedef struct _CBindPair
{
  UInt32 InIndex;
  UInt32 OutIndex;
}CBindPair; 

typedef struct _CFolder
{
  UInt32 NumCoders;
  CCoderInfo *Coders;
  UInt32 NumBindPairs;
  CBindPair *BindPairs;
  UInt32 NumPackStreams; 
  UInt32 *PackStreams;
  CFileSize *UnPackSizes;
  int UnPackCRCDefined;
  UInt32 UnPackCRC;

  UInt32 NumUnPackStreams;
}CFolder;

typedef struct _CFileItem
{
  CFileSize Size;
  UInt32 FileCRC;
  char *Name;
  Byte IsFileCRCDefined;
  Byte HasStream;
  Byte IsDirectory;
  Byte IsAnti;
}CFileItem;


typedef struct _CArchiveDatabase
{
  UInt32 NumPackStreams;
  CFileSize *PackSizes;
  Byte *PackCRCsDefined;
  UInt32 *PackCRCs;
  UInt32 NumFolders;
  CFolder *Folders;
  UInt32 NumFiles;
  CFileItem *Files;
}CArchiveDatabase; 

typedef struct _CInArchiveInfo
{
  CFileSize StartPositionAfterHeader; 
  CFileSize DataStartPosition;
}CInArchiveInfo;

typedef struct _CArchiveDatabaseEx
{
  CArchiveDatabase Database;
  CInArchiveInfo ArchiveInfo;
  UInt32 *FolderStartPackStreamIndex;
  CFileSize *PackStreamStartPositions;
  UInt32 *FolderStartFileIndex;
  UInt32 *FileIndexToFolderIndexMap;
}CArchiveDatabaseEx;	

typedef struct _ISzInStream
{
	SZ_RESULT (*Read)(void *object, void *buffer, size_t size, size_t *processedSize);
	SZ_RESULT (*Seek)(void *object, CFileSize pos);
	
} ISzInStream;
	
typedef struct _file_stream
{
	ISzInStream InStream;
	FILE *File;	
	unsigned int iPos;
	unsigned int iFileSize;
	const char* vBuffer;

	_file_stream()
	{
		iPos	= 0;
		vBuffer = 0;
		iFileSize = 0;
	}
} file_stream;

typedef struct _CSzState
{
  Byte *Data;
  size_t Size;
}CSzData;

typedef struct _CLzmaProperties
{
  int lc;
  int lp;
  int pb;
  #ifdef _LZMA_OUT_READ
  UInt32 DictionarySize;
  #endif
}CLzmaProperties;

typedef struct _CLzmaDecoderState
{
  CLzmaProperties Properties;
  CProb *Probs;

  #ifdef _LZMA_IN_CB
  const unsigned char *Buffer;
  const unsigned char *BufferLim;
  #endif

  #ifdef _LZMA_OUT_READ
  unsigned char *Dictionary;
  UInt32 Range;
  UInt32 Code;
  UInt32 DictionaryPos;
  UInt32 GlobalPos;
  UInt32 DistanceLimit;
  UInt32 Reps[4];
  int State;
  int RemainLen;
  unsigned char TempDictionary[4];
  #endif
} CLzmaDecoderState;
	
SZ_RESULT SzFileReadImp(void *object, void *buffer, size_t maxRequiredSize, size_t *processedSize);
SZ_RESULT SzFileSeekImp(void *object, CFileSize pos);
void *SzAlloc(size_t size);
void SzFree(void *address);

void *SzAllocTemp(size_t size);
void SzFreeTemp(void *address);

struct pack_file
{
	unsigned int	file_size_;
	void*			buffer_;
		
	pack_file()
	{
		file_size_ = 0;
		buffer_ = 0;
	}
	virtual ~pack_file()
	{
		free(buffer_);
	}
};


class zip_pack
{	
	CMethodID k_Copy;// = { { 0x0 }, 1 };
	CMethodID k_LZMA;// = { { 0x3, 0x1, 0x1 }, 3 };
	UInt32 g_CrcTable[256];
	Byte kUtf8Limits[5];// = { 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };
	Byte k7zSignature[k7zSignatureSize];
	
	ISzAlloc			allocImp;
	ISzAlloc			allocTempImp;

	CArchiveDatabaseEx	m_db;
	file_stream			m_archiveStream;

	typedef std::map<QString, std::shared_ptr<pack_file>> mmap;

	mmap				archive_files_;
	
public:	

	zip_pack();
	virtual ~zip_pack();
	
	bool open(const char* _data, unsigned int _size);
	void close();

	const mmap& get_files() const;
		
protected:
		
	SZ_RESULT SzExtract(
		ISzInStream *inStream, 
		CArchiveDatabaseEx *db,
		UInt32 fileIndex,         /* index of file */
		UInt32 *blockIndex,       /* index of solid block */
		Byte **outBuffer,         /* pointer to pointer to output buffer (allocated with allocMain) */
		size_t *outBufferSize,    /* buffer size for output buffer */
		size_t *offset,           /* offset of stream for required file in *outBuffer */
		size_t *outSizeProcessed, /* size of file in *outBuffer */
		ISzAlloc *allocMain,
		ISzAlloc *allocTemp);
	SZ_RESULT SzReadAndDecodePackedStreams2(
		ISzInStream *inStream, 
		CSzData *sd,
		CSzByteBuffer *outBuffer,
		CFileSize baseOffset, 
		CArchiveDatabase *db,
		CFileSize **unPackSizes,
		Byte **digestsDefined,
		UInt32 **digests,
		#ifndef _LZMA_IN_CB
		Byte **inBuffer,
		#endif
		ISzAlloc *allocTemp);
	SZ_RESULT SzReadAndDecodePackedStreams(
		ISzInStream *inStream, 
		CSzData *sd,
		CSzByteBuffer *outBuffer,
		CFileSize baseOffset, 
		ISzAlloc *allocTemp);
	SZ_RESULT SzArchiveOpen2(
		ISzInStream *inStream, 
		CArchiveDatabaseEx *db,
		ISzAlloc *allocMain, 
		ISzAlloc *allocTemp);
	SZ_RESULT SzArchiveOpen(
		ISzInStream *inStream, 
		CArchiveDatabaseEx *db,
		ISzAlloc *allocMain, 
		ISzAlloc *allocTemp);
	SZ_RESULT SzReadHashDigests(
		CSzData *sd, 
		size_t numItems,
		Byte **digestsDefined, 
		UInt32 **digests, 
		void * (*allocFunc)(size_t size));
	SZ_RESULT SzReadPackInfo(
		CSzData *sd, 
		CFileSize *dataOffset,
		UInt32 *numPackStreams,
		CFileSize **packSizes,
		Byte **packCRCsDefined,
		UInt32 **packCRCs,
		void * (*allocFunc)(size_t size));
	SZ_RESULT SzReadUnPackInfo(
		CSzData *sd, 
		UInt32 *numFolders,
		CFolder **folders,  /* for allocFunc */
		void * (*allocFunc)(size_t size),
		ISzAlloc *allocTemp);
	SZ_RESULT SzReadSubStreamsInfo(
		CSzData *sd, 
		UInt32 numFolders,
		CFolder *folders,
		UInt32 *numUnPackStreams,
		CFileSize **unPackSizes,
		Byte **digestsDefined,
		UInt32 **digests,
		ISzAlloc *allocTemp);
	SZ_RESULT SzReadStreamsInfo(
		CSzData *sd, 
		CFileSize *dataOffset,
		CArchiveDatabase *db,
		UInt32 *numUnPackStreams,
		CFileSize **unPackSizes, /* allocTemp */
		Byte **digestsDefined,   /* allocTemp */
		UInt32 **digests,        /* allocTemp */
		void * (*allocFunc)(size_t size),
		ISzAlloc *allocTemp);
	SZ_RESULT SzReadHeader2(
		CSzData *sd, 
		CArchiveDatabaseEx *db,   /* allocMain */
		CFileSize **unPackSizes,  /* allocTemp */
		Byte **digestsDefined,    /* allocTemp */
		UInt32 **digests,         /* allocTemp */
		Byte **emptyStreamVector, /* allocTemp */
		Byte **emptyFileVector,   /* allocTemp */
		ISzAlloc *allocMain, 
		ISzAlloc *allocTemp);
	SZ_RESULT SzReadHeader(
		CSzData *sd, 
		CArchiveDatabaseEx *db, 
		ISzAlloc *allocMain, 
		ISzAlloc *allocTemp);
	SZ_RESULT SzDecode(const CFileSize *packSizes, const CFolder *folder,
		#ifdef _LZMA_IN_CB
		ISzInStream *inStream,
		#else
		const Byte *inBuffer,
		#endif
		Byte *outBuffer, size_t outSize, 
		size_t *outSizeProcessed, ISzAlloc *allocMain);
	int LzmaDecode(CLzmaDecoderState *vs,
		#ifdef _LZMA_IN_CB
		ILzmaInCallback *InCallback,
		#else
		const unsigned char *inStream, SizeT inSize, SizeT *inSizeProcessed,
		#endif
		unsigned char *outStream, SizeT outSize, SizeT *outSizeProcessed);
	SZ_RESULT SafeReadDirect(ISzInStream *inStream, Byte *data, size_t size);
	SZ_RESULT SafeReadDirectByte(ISzInStream *inStream, Byte *data);
	SZ_RESULT SafeReadDirectUInt32(ISzInStream *inStream, UInt32 *value);
	SZ_RESULT SafeReadDirectUInt64(ISzInStream *inStream, UInt64 *value);
	SZ_RESULT SzReadByte(CSzData *sd, Byte *b);
	SZ_RESULT SzReadBytes(CSzData *sd, Byte *data, size_t size);
	SZ_RESULT SzReadUInt32(CSzData *sd, UInt32 *value);
	SZ_RESULT SzReadNumber(CSzData *sd, UInt64 *value);
	SZ_RESULT SzReadSize(CSzData *sd, CFileSize *value);
	SZ_RESULT SzReadNumber32(CSzData *sd, UInt32 *value);
	SZ_RESULT SzReadID(CSzData *sd, UInt64 *value) ;
	SZ_RESULT SzSkeepDataSize(CSzData *sd, UInt64 size);
	SZ_RESULT SzSkeepData(CSzData *sd);
	SZ_RESULT SzReadArchiveProperties(CSzData *sd);
	SZ_RESULT SzWaitAttribute(CSzData *sd, UInt64 attribute);
	SZ_RESULT SzReadBoolVector(CSzData *sd, size_t numItems, Byte **v, void * (*allocFunc)(size_t size));
	SZ_RESULT SzReadBoolVector2(CSzData *sd, size_t numItems, Byte **v, void * (*allocFunc)(size_t size));
	SZ_RESULT SzReadSwitch(CSzData *sd);
	SZ_RESULT SzGetNextFolderItem(CSzData *sd, CFolder *folder, void * (*allocFunc)(size_t size));
	SZ_RESULT SzReadFileNames(CSzData *sd, UInt32 numFiles, CFileItem *files, void * (*allocFunc)(size_t size));
	SZ_RESULT MySzInAlloc(void **p, size_t size, void * (*allocFunc)(size_t size));
	SZ_RESULT SzArDbExFill(CArchiveDatabaseEx *db, void * (*allocFunc)(size_t size));
	void	  SzArDbExInit(CArchiveDatabaseEx *db);
	void	  SzArDbExFree(CArchiveDatabaseEx *db, void (*freeFunc)(void *));
	CFileSize SzArDbGetFolderStreamPos(CArchiveDatabaseEx *db, UInt32 folderIndex, UInt32 indexInFolder);
	CFileSize SzArDbGetFolderFullPackSize(CArchiveDatabaseEx *db, UInt32 folderIndex);
	int		  TestSignatureCandidate(Byte *testBytes);
	void	  InitCrcTable();
	CFileSize SzFolderGetUnPackSize(CFolder *folder);
	int		  SzByteBufferCreate(CSzByteBuffer *buffer, size_t newCapacity, void * (*allocFunc)(size_t size));
	int		  CrcVerifyDigest(UInt32 digest, const void *data, size_t size);
	void	  SzArchiveDatabaseInit(CArchiveDatabase *db);
	void	  SzArchiveDatabaseFree(CArchiveDatabase *db, void (*freeFunc)(void *));
	void	  CrcInit(UInt32 *crc) { *crc = 0xFFFFFFFF; };
	void	  CrcUpdateUInt64(UInt32 *crc, UInt64 v);
	void	  CrcUpdateUInt32(UInt32 *crc, UInt32 v);
	UInt32	  CrcGetDigest(UInt32 *crc);
	void	  SzByteBufferFree(CSzByteBuffer *buffer, void (*freeFunc)(void *));
	void	  CrcUpdateByte(UInt32 *crc, Byte b);
	void	  SzCoderInfoInit(CCoderInfo *coder);
	int		  SzFolderFindBindPairForInStream(CFolder *folder, UInt32 inStreamIndex);
	void	  SzByteBufferInit(CSzByteBuffer *buffer);
	void	  SzFolderInit(CFolder *folder);
	UInt32	  SzFolderGetNumOutStreams(CFolder *folder);
	void	  SzFileInit(CFileItem *fileItem);
	int		  AreMethodsEqual(CMethodID *a1, CMethodID *a2);
	void	  SzFolderFree(CFolder *folder, void (*freeFunc)(void *p));
	void	  SzFileFree(CFileItem *fileItem, void (*freeFunc)(void *p));
	void	  SzCoderInfoFree(CCoderInfo *coder, void (*freeFunc)(void *p));
	int		  SzFolderFindBindPairForOutStream(CFolder *folder, UInt32 outStreamIndex);
	UInt32	  CrcCalculateDigest(const void *data, size_t size);
	void	  CrcUpdate(UInt32 *crc, const void *data, size_t size);
	int		  LzmaDecodeProperties(CLzmaProperties *propsRes, const unsigned char *propsData, int size);

};
