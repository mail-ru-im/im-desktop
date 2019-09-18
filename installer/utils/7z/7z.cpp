// SkinZip.cpp: implementation of the CSkinZip class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "7z.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#define DEFAULT_NAME_SKIN "DEFAULT"
#define RINOM(x) { if((x) == 0) return SZE_OUTOFMEMORY; }

zip_pack::zip_pack()
{
	k_Copy.ID[0] = 0x0;
	k_Copy.IDSize = 1;

	k_LZMA.ID[0] = 0x3;
	k_LZMA.ID[1] = 0x1;
	k_LZMA.ID[2] = 0x1;
	k_LZMA.IDSize = 3;
	
	k7zSignature[0] = '7';
	k7zSignature[1] = 'z';
	k7zSignature[2] = 0xBC;
	k7zSignature[3] = 0xAF;
	k7zSignature[4] = 0x27;
	k7zSignature[5] = 0x1C;

	kUtf8Limits[0] = 0xC0;
	kUtf8Limits[1] = 0xE0;
	kUtf8Limits[2] = 0xF0;
	kUtf8Limits[3] = 0xF8;
	kUtf8Limits[4] = 0xFC;
}

zip_pack::~zip_pack()
{
	close();
}

SZ_RESULT SzFileReadImpMemory(void *object, void *buffer, size_t size, size_t *processedSize)
{  
	file_stream *s = (file_stream *)object;
	
	if( (s->iPos + size) > s->iFileSize)
		return SZE_FAIL;

	memcpy(buffer, (BYTE*)(&s->vBuffer[s->iPos]), size);
	s->iPos += size;
	
	if (processedSize != 0)
		*processedSize = size;
	return SZ_OK;
}

SZ_RESULT SzFileSeekImpMemory(void *object, CFileSize pos)
{
  file_stream *s = (file_stream *)object;
  s->iPos = pos;
 
  if (pos < s->iFileSize)
    return SZ_OK;
  return SZE_FAIL;
}

SZ_RESULT SzFileReadImp(void *object, void *buffer, size_t size, size_t *processedSize)
{
  file_stream *s = (file_stream *)object;
  size_t processedSizeLoc = fread(buffer, 1, size, s->File);
  
  if (processedSize != 0)
    *processedSize = processedSizeLoc;
  return SZ_OK;
}

SZ_RESULT SzFileSeekImp(void *object, CFileSize pos)
{
  file_stream *s = (file_stream *)object;
  int res = fseek(s->File, (long)pos, SEEK_SET);
  if (res == 0)
    return SZ_OK;
  return SZE_FAIL;
}


const zip_pack::mmap& zip_pack::get_files() const
{
	return archive_files_;
}

void zip_pack::close()
{
	archive_files_.clear();
}


bool zip_pack::open(const char* _data, unsigned int _size)
{
	m_archiveStream.vBuffer = _data;
	m_archiveStream.iFileSize =	_size;
	m_archiveStream.InStream.Read = SzFileReadImpMemory;
	m_archiveStream.InStream.Seek = SzFileSeekImpMemory;

	allocImp.Alloc = SzAlloc;
	allocImp.Free = SzFree;
	allocTempImp.Alloc = SzAllocTemp;
	allocTempImp.Free = SzFreeTemp;
	InitCrcTable();
	SzArDbExInit(&m_db);

	auto res = SzArchiveOpen(&m_archiveStream.InStream, &m_db, &allocImp, &allocTempImp);
	
	UInt32 blockIndex = 0xFFFFFFFF; // it can have any value before first call (if outBuffer = 0) 
	Byte *outBuffer = 0; // it must be 0 before first call for each new archive. 
	size_t outBufferSize = 0;  // it can have any value before first call (if outBuffer = 0) 
	
	for (UInt32 i = 0; i < m_db.Database.NumFiles; ++i)
	{
		size_t offset;
		size_t outSizeProcessed = 0;
		CFileItem *f = m_db.Database.Files + i;
		QString sPath = f->Name;
	
		if (!f->IsDirectory)
		{
			res = SzExtract(&m_archiveStream.InStream, &m_db, i, &blockIndex, &outBuffer, &outBufferSize, &offset, &outSizeProcessed, &allocImp, &allocTempImp);
			
			if (res != SZ_OK)
				break;
						
			auto stPackFile = std::make_shared<pack_file>();
			stPackFile->file_size_ = outSizeProcessed;
			stPackFile->buffer_	= (BYTE*) malloc( outSizeProcessed ); 
			memcpy(stPackFile->buffer_, outBuffer + offset, outSizeProcessed);

			QString file_name = f->Name;
			archive_files_.insert(std::make_pair(file_name.toLower(), stPackFile));
		}
	}
			
	free(outBuffer);
	
	SzArDbExFree(&m_db, allocImp.Free);
	return	(res == SZ_OK);
}

///////////// defined other places
SZ_RESULT zip_pack::SzExtract(
    ISzInStream *inStream, 
    CArchiveDatabaseEx *db,
    UInt32 fileIndex,
    UInt32 *blockIndex,
    Byte **outBuffer, 
    size_t *outBufferSize,
    size_t *offset, 
    size_t *outSizeProcessed, 
    ISzAlloc *allocMain,
    ISzAlloc *allocTemp)
{
  UInt32 folderIndex = db->FileIndexToFolderIndexMap[fileIndex];
  SZ_RESULT res = SZ_OK;
  *offset = 0;
  *outSizeProcessed = 0;
  if (folderIndex == (UInt32)-1)
  {
    allocMain->Free(*outBuffer);
    *blockIndex = folderIndex;
    *outBuffer = 0;
    *outBufferSize = 0;
    return SZ_OK;
  }

  if (*outBuffer == 0 || *blockIndex != folderIndex)
  {
    //CFolder *folder1 = db->Database.Folders + folderIndex + 1;
	CFolder *folder = db->Database.Folders + folderIndex;
    CFileSize unPackSize = SzFolderGetUnPackSize(folder);
	//CFileSize unPackSize1 = SzFolderGetUnPackSize(folder1);
    #ifndef _LZMA_IN_CB
    CFileSize packSize = SzArDbGetFolderFullPackSize(db, folderIndex);
    Byte *inBuffer = 0;
    size_t processedSize;
    #endif
    *blockIndex = folderIndex;
    allocMain->Free(*outBuffer);
    *outBuffer = 0;
    

    RINOK(inStream->Seek(inStream, SzArDbGetFolderStreamPos(db, folderIndex, 0)));
    
    #ifndef _LZMA_IN_CB
    if (packSize != 0)
    {
      inBuffer = (Byte *)allocTemp->Alloc((size_t)packSize);
      if (inBuffer == 0)
        return SZE_OUTOFMEMORY;
    }
    res = inStream->Read(inStream, inBuffer, (size_t)packSize, &processedSize);
    if (res == SZ_OK && processedSize != (size_t)packSize)
      res = SZE_FAIL;
    #endif
    if (res == SZ_OK)
    {
      *outBufferSize = (size_t)unPackSize;
      if (unPackSize != 0)
      {
        *outBuffer = (Byte *)allocMain->Alloc((size_t)unPackSize);
        if (*outBuffer == 0)
          res = SZE_OUTOFMEMORY;
      }
      if (res == SZ_OK)
      {
        size_t outRealSize;
        res = SzDecode(db->Database.PackSizes + 
          db->FolderStartPackStreamIndex[folderIndex], folder, 
          #ifdef _LZMA_IN_CB
          inStream,
          #else
          inBuffer, 
          #endif
          *outBuffer, (size_t)unPackSize, &outRealSize, allocTemp);
        if (res == SZ_OK)
        {
          if (outRealSize == (size_t)unPackSize)
          {
            if (folder->UnPackCRCDefined)
            {
              if (!CrcVerifyDigest(folder->UnPackCRC, *outBuffer, (size_t)unPackSize))
                res = SZE_FAIL;
            }
          }
          else
            res = SZE_FAIL;
        }
      }
    }
    #ifndef _LZMA_IN_CB
    allocTemp->Free(inBuffer);
    #endif
  }
  if (res == SZ_OK)
  {
    UInt32 i; 
    CFileItem *fileItem = db->Database.Files + fileIndex;
    *offset = 0;
    for(i = db->FolderStartFileIndex[folderIndex]; i < fileIndex; i++)
      *offset += (UInt32)db->Database.Files[i].Size;
    *outSizeProcessed = (size_t)fileItem->Size;
    if (*offset + *outSizeProcessed > *outBufferSize)
      return SZE_FAIL;
    {
      if (fileItem->IsFileCRCDefined)
      {
        if (!CrcVerifyDigest(fileItem->FileCRC, *outBuffer + *offset, *outSizeProcessed))
          res = SZE_FAIL;
      }
    }
  }
  return res;
}


SZ_RESULT zip_pack::SafeReadDirect(ISzInStream *inStream, Byte *data, size_t size)
{
  #ifdef _LZMA_IN_CB
  while (size > 0)
  {
    Byte *inBuffer;
    size_t processedSize;
    RINOK(inStream->Read(inStream, (void **)&inBuffer, size, &processedSize));
    if (processedSize == 0 || processedSize > size)
      return SZE_FAIL;
    size -= processedSize;
    do
    {
      *data++ = *inBuffer++;
    }
    while (--processedSize != 0);
  }
  #else
  size_t processedSize;
  RINOK(inStream->Read(inStream, data, size, &processedSize));
  if (processedSize != size)
    return SZE_FAIL;
  #endif
  return SZ_OK;
}
//////////////////////////////////////////////////////////////
SZ_RESULT zip_pack::SafeReadDirectByte(ISzInStream *inStream, Byte *data)
{
  return SafeReadDirect(inStream, data, 1);
}
//////////////////////////////////////////////////////////////
SZ_RESULT zip_pack::SafeReadDirectUInt32(ISzInStream *inStream, UInt32 *value)
{
  int i;
  *value = 0;
  for (i = 0; i < 4; i++)
  {
    Byte b;
    RINOK(SafeReadDirectByte(inStream, &b));
    *value |= ((UInt32)b << (8 * i));
  }
  return SZ_OK;
}
///////////////////////////////////////////////////////////////
SZ_RESULT zip_pack::SafeReadDirectUInt64(ISzInStream *inStream, UInt64 *value)
{
  int i;
  *value = 0;
  for (i = 0; i < 8; i++)
  {
    Byte b;
    RINOK(SafeReadDirectByte(inStream, &b));
    *value |= ((UInt32)b << (8 * i));
  }
  return SZ_OK;
}
///////////////////////////////////////////////////////////////
SZ_RESULT zip_pack::SzReadAndDecodePackedStreams2(
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
    ISzAlloc *allocTemp)
{

  UInt32 numUnPackStreams = 0;
  CFileSize dataStartPos;
  CFolder *folder;
  #ifndef _LZMA_IN_CB
  CFileSize packSize = 0;
  UInt32 i = 0;
  #endif
  CFileSize unPackSize;
  size_t outRealSize;
  SZ_RESULT res;

  RINOK(SzReadStreamsInfo(sd, &dataStartPos, db,
      &numUnPackStreams,  unPackSizes, digestsDefined, digests, 
      allocTemp->Alloc, allocTemp));
  
  dataStartPos += baseOffset;
  if (db->NumFolders != 1)
    return SZE_ARCHIVE_ERROR;

  folder = db->Folders;
  unPackSize = SzFolderGetUnPackSize(folder);
  
  RINOK(inStream->Seek(inStream, dataStartPos));

  #ifndef _LZMA_IN_CB
  for (i = 0; i < db->NumPackStreams; i++)
    packSize += db->PackSizes[i];

  RINOK(MySzInAlloc((void **)inBuffer, (size_t)packSize, allocTemp->Alloc));

  RINOK(SafeReadDirect(inStream, *inBuffer, (size_t)packSize));
  #endif

  if (!SzByteBufferCreate(outBuffer, (size_t)unPackSize, allocTemp->Alloc))
    return SZE_OUTOFMEMORY;
  
  res = SzDecode(db->PackSizes, folder, 
          #ifdef _LZMA_IN_CB
          inStream,
          #else
          *inBuffer, 
          #endif
          outBuffer->Items, (size_t)unPackSize,
          &outRealSize, allocTemp);
  RINOK(res)
  if (outRealSize != (UInt32)unPackSize)
    return SZE_FAIL;
  if (folder->UnPackCRCDefined)
    if (!CrcVerifyDigest(folder->UnPackCRC, outBuffer->Items, (size_t)unPackSize))
      return SZE_FAIL;
  return SZ_OK;
}
///////////////////////////////////////////////////////////////
SZ_RESULT zip_pack::SzReadAndDecodePackedStreams(
    ISzInStream *inStream, 
    CSzData *sd,
    CSzByteBuffer *outBuffer,
    CFileSize baseOffset, 
    ISzAlloc *allocTemp)
{
  CArchiveDatabase db;
  CFileSize *unPackSizes = 0;
  Byte *digestsDefined = 0;
  UInt32 *digests = 0;
  #ifndef _LZMA_IN_CB
  Byte *inBuffer = 0;
  #endif
  SZ_RESULT res;
  SzArchiveDatabaseInit(&db);
  res = SzReadAndDecodePackedStreams2(inStream, sd, outBuffer, baseOffset, 
    &db, &unPackSizes, &digestsDefined, &digests, 
    #ifndef _LZMA_IN_CB
    &inBuffer,
    #endif
    allocTemp);
  SzArchiveDatabaseFree(&db, allocTemp->Free);
  allocTemp->Free(unPackSizes);
  allocTemp->Free(digestsDefined);
  allocTemp->Free(digests);
  #ifndef _LZMA_IN_CB
  allocTemp->Free(inBuffer);
  #endif
  return res;
}
///////////////////////////////////////////////////////////
SZ_RESULT zip_pack::SzArchiveOpen2(
    ISzInStream *inStream, 
    CArchiveDatabaseEx *db,
    ISzAlloc *allocMain, 
    ISzAlloc *allocTemp)
{
  Byte signature[k7zSignatureSize];
  Byte version;
  UInt32 crcFromArchive;
  UInt64 nextHeaderOffset;
  UInt64 nextHeaderSize;
  UInt32 nextHeaderCRC;
  UInt32 crc;
  CFileSize pos = 0;
  CSzByteBuffer buffer;
  CSzData sd;
  SZ_RESULT res;

  RINOK(SafeReadDirect(inStream, signature, k7zSignatureSize));

  if (!TestSignatureCandidate(signature))
    return SZE_ARCHIVE_ERROR;

  /*
  db.Clear();
  db.ArchiveInfo.StartPosition = _arhiveBeginStreamPosition;
  */
  RINOK(SafeReadDirectByte(inStream, &version));
  if (version != k7zMajorVersion)
    return SZE_ARCHIVE_ERROR;
  RINOK(SafeReadDirectByte(inStream, &version));

  RINOK(SafeReadDirectUInt32(inStream, &crcFromArchive));

  CrcInit(&crc);
  RINOK(SafeReadDirectUInt64(inStream, &nextHeaderOffset));
  CrcUpdateUInt64(&crc, nextHeaderOffset);
  RINOK(SafeReadDirectUInt64(inStream, &nextHeaderSize));
  CrcUpdateUInt64(&crc, nextHeaderSize);
  RINOK(SafeReadDirectUInt32(inStream, &nextHeaderCRC));
  CrcUpdateUInt32(&crc, nextHeaderCRC);

  pos = k7zStartHeaderSize;
  db->ArchiveInfo.StartPositionAfterHeader = pos;
  
  if (CrcGetDigest(&crc) != crcFromArchive)
    return SZE_ARCHIVE_ERROR;

  if (nextHeaderSize == 0)
    return SZ_OK;

  RINOK(inStream->Seek(inStream, (CFileSize)(pos + nextHeaderOffset)));

  if (!SzByteBufferCreate(&buffer, (size_t)nextHeaderSize, allocTemp->Alloc))
    return SZE_OUTOFMEMORY;

  res = SafeReadDirect(inStream, buffer.Items, (size_t)nextHeaderSize);
  if (res == SZ_OK)
  {
    if (CrcVerifyDigest(nextHeaderCRC, buffer.Items, (UInt32)nextHeaderSize))
    {
      while (1)
      {
        UInt64 type;
        sd.Data = buffer.Items;
        sd.Size = buffer.Capacity;
        res = SzReadID(&sd, &type);
        if (res != SZ_OK)
          break;
        if (type == k7zIdHeader)
        {
          res = SzReadHeader(&sd, db, allocMain, allocTemp);
          break;
        }
        if (type != k7zIdEncodedHeader)
        {
          res = SZE_ARCHIVE_ERROR;
          break;
        }
        {
          CSzByteBuffer outBuffer;
          res = SzReadAndDecodePackedStreams(inStream, &sd, &outBuffer, 
              db->ArchiveInfo.StartPositionAfterHeader, 
              allocTemp);
          if (res != SZ_OK)
          {
            SzByteBufferFree(&outBuffer, allocTemp->Free);
            break;
          }
          SzByteBufferFree(&buffer, allocTemp->Free);
          buffer.Items = outBuffer.Items;
          buffer.Capacity = outBuffer.Capacity;
        }
      }
    }
  }
  SzByteBufferFree(&buffer, allocTemp->Free);
  return res;
}
/////////////////////////////////////////////////////////
void zip_pack::SzByteBufferFree(CSzByteBuffer *buffer, void (*freeFunc)(void *))
{
  freeFunc(buffer->Items);
  buffer->Items = 0;
  buffer->Capacity = 0;
}
/////////////////////////////////////////////////////////
UInt32 zip_pack::CrcGetDigest(UInt32 *crc) 
{ 
	return *crc ^ 0xFFFFFFFF; 
} 
/////////////////////////////////////////////////////////
void zip_pack::CrcUpdateUInt64(UInt32 *crc, UInt64 v)
{
  int i;
  for (i = 0; i < 8; i++)
  {
    CrcUpdateByte(crc, (Byte)(v));
    v >>= 8;
  }
}
/////////////////////////////////////////////////////////
void  zip_pack::CrcUpdateByte(UInt32 *crc, Byte b)
{
  *crc = g_CrcTable[((Byte)(*crc)) ^ b] ^ (*crc >> 8);
} 
/////////////////////////////////////////////////////////
void zip_pack::CrcUpdateUInt32(UInt32 *crc, UInt32 v)
{
  int i;
  for (i = 0; i < 4; i++)
    CrcUpdateByte(crc, (Byte)(v >> (8 * i)));
} 
/////////////////////////////////////////////////////////
SZ_RESULT zip_pack::SzArchiveOpen(
    ISzInStream *inStream, 
    CArchiveDatabaseEx *db,
    ISzAlloc *allocMain, 
    ISzAlloc *allocTemp)
{
  SZ_RESULT res = SzArchiveOpen2(inStream, db, allocMain, allocTemp);
  if (res != SZ_OK)
    SzArDbExFree(db, allocMain->Free);
  return res;
}
/////////////////////////////////////////////////////////
SZ_RESULT zip_pack::SzReadByte(CSzData *sd, Byte *b)
{
  if (sd->Size == 0)
    return SZE_ARCHIVE_ERROR;
  sd->Size--;
  *b = *sd->Data++;
  return SZ_OK;
}
/////////////////////////////////////////////////////////
SZ_RESULT zip_pack::SzReadBytes(CSzData *sd, Byte *data, size_t size)
{
  size_t i;
  for (i = 0; i < size; i++)
  {
    RINOK(SzReadByte(sd, data + i));
  }
  return SZ_OK;
}
/////////////////////////////////////////////////////////
SZ_RESULT zip_pack::SzReadUInt32(CSzData *sd, UInt32 *value)
{
  int i;
  *value = 0;
  for (i = 0; i < 4; i++)
  {
    Byte b;
    RINOK(SzReadByte(sd, &b));
    *value |= ((UInt32)(b) << (8 * i));
  }
  return SZ_OK;
}
/////////////////////////////////////////////////////////
SZ_RESULT zip_pack::SzReadNumber(CSzData *sd, UInt64 *value)
{
  Byte firstByte;
  Byte mask = 0x80;
  int i;
  RINOK(SzReadByte(sd, &firstByte));
  *value = 0;
  for (i = 0; i < 8; i++)
  {
    Byte b;
    if ((firstByte & mask) == 0)
    {
      UInt64 highPart = firstByte & (mask - 1);
      *value += (highPart << (8 * i));
      return SZ_OK;
    }
    RINOK(SzReadByte(sd, &b));
    *value |= ((UInt64)b << (8 * i));
    mask >>= 1;
  }
  return SZ_OK;
}
/////////////////////////////////////////////////////////
SZ_RESULT zip_pack::SzReadSize(CSzData *sd, CFileSize *value)
{
  UInt64 value64;
  RINOK(SzReadNumber(sd, &value64));
  *value = (CFileSize)value64;
  return SZ_OK;
}
/////////////////////////////////////////////////////////
SZ_RESULT zip_pack::SzReadNumber32(CSzData *sd, UInt32 *value)
{
  UInt64 value64;
  RINOK(SzReadNumber(sd, &value64));
  if (value64 >= 0x80000000)
    return SZE_NOTIMPL;
  if (value64 >= ((UInt64)(1) << ((sizeof(size_t) - 1) * 8 + 2)))
    return SZE_NOTIMPL;
  *value = (UInt32)value64;
  return SZ_OK;
}
/////////////////////////////////////////////////////////
SZ_RESULT zip_pack::SzReadID(CSzData *sd, UInt64 *value) 
{ 
  return SzReadNumber(sd, value); 
}
/////////////////////////////////////////////////////////
SZ_RESULT zip_pack::SzSkeepDataSize(CSzData *sd, UInt64 size)
{
  if (size > sd->Size)
    return SZE_ARCHIVE_ERROR;
  sd->Size -= (size_t)size;
  sd->Data += (size_t)size;
  return SZ_OK;
}
/////////////////////////////////////////////////////////
SZ_RESULT zip_pack::SzSkeepData(CSzData *sd)
{
  UInt64 size;
  RINOK(SzReadNumber(sd, &size));
  return SzSkeepDataSize(sd, size);
}
/////////////////////////////////////////////////////////
SZ_RESULT zip_pack::SzReadArchiveProperties(CSzData *sd)
{
  while(1)
  {
    UInt64 type;
    RINOK(SzReadID(sd, &type));
    if (type == k7zIdEnd)
      break;
    SzSkeepData(sd);
  }
  return SZ_OK;
}
/////////////////////////////////////////////////////////
SZ_RESULT zip_pack::SzWaitAttribute(CSzData *sd, UInt64 attribute)
{
  while(1)
  {
    UInt64 type;
    RINOK(SzReadID(sd, &type));
    if (type == attribute)
      return SZ_OK;
    if (type == k7zIdEnd)
      return SZE_ARCHIVE_ERROR;
    RINOK(SzSkeepData(sd));
  }
}
/////////////////////////////////////////////////////////
SZ_RESULT zip_pack::SzReadBoolVector(CSzData *sd, size_t numItems, Byte **v, void * (*allocFunc)(size_t size))
{
  Byte b = 0;
  Byte mask = 0;
  size_t i;
  RINOK(MySzInAlloc((void **)v, numItems * sizeof(Byte), allocFunc));
  for(i = 0; i < numItems; i++)
  {
    if (mask == 0)
    {
      RINOK(SzReadByte(sd, &b));
      mask = 0x80;
    }
    (*v)[i] = (Byte)(((b & mask) != 0) ? 1 : 0);
    mask >>= 1;
  }
  return SZ_OK;
}
/////////////////////////////////////////////////////////
SZ_RESULT zip_pack::SzReadBoolVector2(CSzData *sd, size_t numItems, Byte **v, void * (*allocFunc)(size_t size))
{
  Byte allAreDefined;
  size_t i;
  RINOK(SzReadByte(sd, &allAreDefined));
  if (allAreDefined == 0)
    return SzReadBoolVector(sd, numItems, v, allocFunc);
  RINOK(MySzInAlloc((void **)v, numItems * sizeof(Byte), allocFunc));
  for(i = 0; i < numItems; i++)
    (*v)[i] = 1;
  return SZ_OK;
}
/////////////////////////////////////////////////////////
SZ_RESULT zip_pack::SzReadHashDigests(
    CSzData *sd, 
    size_t numItems,
    Byte **digestsDefined, 
    UInt32 **digests, 
    void * (*allocFunc)(size_t size))
{
  size_t i;
  RINOK(SzReadBoolVector2(sd, numItems, digestsDefined, allocFunc));
  RINOK(MySzInAlloc((void **)digests, numItems * sizeof(UInt32), allocFunc));
  for(i = 0; i < numItems; i++)
    if ((*digestsDefined)[i])
    {
      RINOK(SzReadUInt32(sd, (*digests) + i));
    }
  return SZ_OK;
}
/////////////////////////////////////////////////////////
SZ_RESULT zip_pack::SzReadPackInfo(
    CSzData *sd, 
    CFileSize *dataOffset,
    UInt32 *numPackStreams,
    CFileSize **packSizes,
    Byte **packCRCsDefined,
    UInt32 **packCRCs,
    void * (*allocFunc)(size_t size))
{
  UInt32 i;
  RINOK(SzReadSize(sd, dataOffset));
  RINOK(SzReadNumber32(sd, numPackStreams));

  RINOK(SzWaitAttribute(sd, k7zIdSize));

  RINOK(MySzInAlloc((void **)packSizes, (size_t)*numPackStreams * sizeof(CFileSize), allocFunc));

  for(i = 0; i < *numPackStreams; i++)
  {
    RINOK(SzReadSize(sd, (*packSizes) + i));
  }

  while(1)
  {
    UInt64 type;
    RINOK(SzReadID(sd, &type));
    if (type == k7zIdEnd)
      break;
    if (type == k7zIdCRC)
    {
      RINOK(SzReadHashDigests(sd, (size_t)*numPackStreams, packCRCsDefined, packCRCs, allocFunc)); 
      continue;
    }
    RINOK(SzSkeepData(sd));
  }
  if (*packCRCsDefined == 0)
  {
    RINOK(MySzInAlloc((void **)packCRCsDefined, (size_t)*numPackStreams * sizeof(Byte), allocFunc));
    RINOK(MySzInAlloc((void **)packCRCs, (size_t)*numPackStreams * sizeof(UInt32), allocFunc));
    for(i = 0; i < *numPackStreams; i++)
    {
      (*packCRCsDefined)[i] = 0;
      (*packCRCs)[i] = 0;
    }
  }
  return SZ_OK;
}
///////////////////////////////////////////////////////
SZ_RESULT zip_pack::SzReadSwitch(CSzData *sd)
{
  Byte external;
  RINOK(SzReadByte(sd, &external));
  return (external == 0) ? SZ_OK: SZE_ARCHIVE_ERROR;
}
///////////////////////////////////////////////////////
SZ_RESULT zip_pack::SzGetNextFolderItem(CSzData *sd, CFolder *folder, void * (*allocFunc)(size_t size))
{
  UInt32 numCoders;
  UInt32 numBindPairs;
  UInt32 numPackedStreams;
  UInt32 i;
  UInt32 numInStreams = 0;
  UInt32 numOutStreams = 0;
  RINOK(SzReadNumber32(sd, &numCoders));
  folder->NumCoders = numCoders;

  RINOK(MySzInAlloc((void **)&folder->Coders, (size_t)numCoders * sizeof(CCoderInfo), allocFunc));

  for (i = 0; i < numCoders; i++)
    SzCoderInfoInit(folder->Coders + i);

  for (i = 0; i < numCoders; i++)
  {
    Byte mainByte;
    CCoderInfo *coder = folder->Coders + i;
    {
      RINOK(SzReadByte(sd, &mainByte));
      coder->MethodID.IDSize = (Byte)(mainByte & 0xF);
      RINOK(SzReadBytes(sd, coder->MethodID.ID, coder->MethodID.IDSize));
      if ((mainByte & 0x10) != 0)
      {
        RINOK(SzReadNumber32(sd, &coder->NumInStreams));
        RINOK(SzReadNumber32(sd, &coder->NumOutStreams));
      }
      else
      {
        coder->NumInStreams = 1;
        coder->NumOutStreams = 1;
      }
      if ((mainByte & 0x20) != 0)
      {
        UInt64 propertiesSize = 0;
        RINOK(SzReadNumber(sd, &propertiesSize));
        if (!SzByteBufferCreate(&coder->Properties, (size_t)propertiesSize, allocFunc))
          return SZE_OUTOFMEMORY;
        RINOK(SzReadBytes(sd, coder->Properties.Items, (size_t)propertiesSize));
      }
    }
    while ((mainByte & 0x80) != 0)
    {
      RINOK(SzReadByte(sd, &mainByte));
      RINOK(SzSkeepDataSize(sd, (mainByte & 0xF)));
      if ((mainByte & 0x10) != 0)
      {
        UInt32 n;
        RINOK(SzReadNumber32(sd, &n));
        RINOK(SzReadNumber32(sd, &n));
      }
      if ((mainByte & 0x20) != 0)
      {
        UInt64 propertiesSize = 0;
        RINOK(SzReadNumber(sd, &propertiesSize));
        RINOK(SzSkeepDataSize(sd, propertiesSize));
      }
    }
    numInStreams += (UInt32)coder->NumInStreams;
    numOutStreams += (UInt32)coder->NumOutStreams;
  }

  numBindPairs = numOutStreams - 1;
  folder->NumBindPairs = numBindPairs;


  RINOK(MySzInAlloc((void **)&folder->BindPairs, (size_t)numBindPairs * sizeof(CBindPair), allocFunc));

  for (i = 0; i < numBindPairs; i++)
  {
    CBindPair *bindPair = folder->BindPairs + i;;
    RINOK(SzReadNumber32(sd, &bindPair->InIndex));
    RINOK(SzReadNumber32(sd, &bindPair->OutIndex)); 
  }

  numPackedStreams = numInStreams - (UInt32)numBindPairs;

  folder->NumPackStreams = numPackedStreams;
  RINOK(MySzInAlloc((void **)&folder->PackStreams, (size_t)numPackedStreams * sizeof(UInt32), allocFunc));

  if (numPackedStreams == 1)
  {
    UInt32 j;
    UInt32 pi = 0;
    for (j = 0; j < numInStreams; j++)
      if (SzFolderFindBindPairForInStream(folder, j) < 0)
      {
        folder->PackStreams[pi++] = j;
        break;
      }
  }
  else
    for(i = 0; i < numPackedStreams; i++)
    {
      RINOK(SzReadNumber32(sd, folder->PackStreams + i));
    }
  return SZ_OK;
}
////////////////////////////////////////////////////////
int zip_pack::SzFolderFindBindPairForInStream(CFolder *folder, UInt32 inStreamIndex)
{
  UInt32 i;
  for(i = 0; i < folder->NumBindPairs; i++)
    if (folder->BindPairs[i].InIndex == inStreamIndex)
      return i;
  return -1;
}
////////////////////////////////////////////////////////
void zip_pack::SzCoderInfoInit(CCoderInfo *coder)
{
  SzByteBufferInit(&coder->Properties);
}
//////////////////////////////////////////////////////// 
void zip_pack::SzByteBufferInit(CSzByteBuffer *buffer)
{
  buffer->Capacity = 0;
  buffer->Items = 0;
}
////////////////////////////////////////////////////////
SZ_RESULT zip_pack::SzReadUnPackInfo(
    CSzData *sd, 
    UInt32 *numFolders,
    CFolder **folders,  /* for allocFunc */
    void * (*allocFunc)(size_t size),
    ISzAlloc *allocTemp)
{
  UInt32 i;
  RINOK(SzWaitAttribute(sd, k7zIdFolder));
  RINOK(SzReadNumber32(sd, numFolders));
  {
    RINOK(SzReadSwitch(sd));


    RINOK(MySzInAlloc((void **)folders, (size_t)*numFolders * sizeof(CFolder), allocFunc));

    for(i = 0; i < *numFolders; i++)
      SzFolderInit((*folders) + i);

    for(i = 0; i < *numFolders; i++)
    {
      RINOK(SzGetNextFolderItem(sd, (*folders) + i, allocFunc));
    }
  }

  RINOK(SzWaitAttribute(sd, k7zIdCodersUnPackSize));

  for(i = 0; i < *numFolders; i++)
  {
    UInt32 j;
    CFolder *folder = (*folders) + i;
    UInt32 numOutStreams = SzFolderGetNumOutStreams(folder);

    RINOK(MySzInAlloc((void **)&folder->UnPackSizes, (size_t)numOutStreams * sizeof(CFileSize), allocFunc));

    for(j = 0; j < numOutStreams; j++)
    {
      RINOK(SzReadSize(sd, folder->UnPackSizes + j));
    }
  }

  while(1)
  {
    UInt64 type;
    RINOK(SzReadID(sd, &type));
    if (type == k7zIdEnd)
      return SZ_OK;
    if (type == k7zIdCRC)
    {
      SZ_RESULT res;
      Byte *crcsDefined = 0;
      UInt32 *crcs = 0;
      res = SzReadHashDigests(sd, *numFolders, &crcsDefined, &crcs, allocTemp->Alloc); 
      if (res == SZ_OK)
      {
        for(i = 0; i < *numFolders; i++)
        {
          CFolder *folder = (*folders) + i;
          folder->UnPackCRCDefined = crcsDefined[i];
          folder->UnPackCRC = crcs[i];
        }
      }
      allocTemp->Free(crcs);
      allocTemp->Free(crcsDefined);
      RINOK(res);
      continue;
    }
    RINOK(SzSkeepData(sd));
  }
}
/////////////////////////////////////////////////////
UInt32 zip_pack::SzFolderGetNumOutStreams(CFolder *folder)
{
  UInt32 result = 0;
  UInt32 i;
  for (i = 0; i < folder->NumCoders; i++)
    result += folder->Coders[i].NumOutStreams;
  return result;
}
/////////////////////////////////////////////////////
void zip_pack::SzFolderInit(CFolder *folder)
{
  folder->NumCoders = 0;
  folder->Coders = 0;
  folder->NumBindPairs = 0;
  folder->BindPairs = 0;
  folder->NumPackStreams = 0;
  folder->PackStreams = 0;
  folder->UnPackSizes = 0;
  folder->UnPackCRCDefined = 0;
  folder->UnPackCRC = 0;
  folder->NumUnPackStreams = 0;
}
/////////////////////////////////////////////////////
SZ_RESULT zip_pack::SzReadSubStreamsInfo(
    CSzData *sd, 
    UInt32 numFolders,
    CFolder *folders,
    UInt32 *numUnPackStreams,
    CFileSize **unPackSizes,
    Byte **digestsDefined,
    UInt32 **digests,
    ISzAlloc *allocTemp)
{
  UInt64 type = 0;
  UInt32 i;
  UInt32 si = 0;
  UInt32 numDigests = 0;

  for(i = 0; i < numFolders; i++)
    folders[i].NumUnPackStreams = 1;
  *numUnPackStreams = numFolders;

  while(1)
  {
    RINOK(SzReadID(sd, &type));
    if (type == k7zIdNumUnPackStream)
    {
      *numUnPackStreams = 0;
      for(i = 0; i < numFolders; i++)
      {
        UInt32 numStreams;
        RINOK(SzReadNumber32(sd, &numStreams));
        folders[i].NumUnPackStreams = numStreams;
        *numUnPackStreams += numStreams;
      }
      continue;
    }
    if (type == k7zIdCRC || type == k7zIdSize)
      break;
    if (type == k7zIdEnd)
      break;
    RINOK(SzSkeepData(sd));
  }

  if (*numUnPackStreams == 0)
  {
    *unPackSizes = 0;
    *digestsDefined = 0;
    *digests = 0;
  }
  else
  {
    *unPackSizes = (CFileSize *)allocTemp->Alloc((size_t)*numUnPackStreams * sizeof(CFileSize));
    RINOM(*unPackSizes);
    *digestsDefined = (Byte *)allocTemp->Alloc((size_t)*numUnPackStreams * sizeof(Byte));
    RINOM(*digestsDefined);
    *digests = (UInt32 *)allocTemp->Alloc((size_t)*numUnPackStreams * sizeof(UInt32));
    RINOM(*digests);
  }

  for(i = 0; i < numFolders; i++)
  {
    /*
    v3.13 incorrectly worked with empty folders
    v4.07: we check that folder is empty
    */
    CFileSize sum = 0;
    UInt32 j;
    UInt32 numSubstreams = folders[i].NumUnPackStreams;
    if (numSubstreams == 0)
      continue;
    if (type == k7zIdSize)
    for (j = 1; j < numSubstreams; j++)
    {
      CFileSize size;
      RINOK(SzReadSize(sd, &size));
      (*unPackSizes)[si++] = size;
      sum += size;
    }
    (*unPackSizes)[si++] = SzFolderGetUnPackSize(folders + i) - sum;
  }
  if (type == k7zIdSize)
  {
    RINOK(SzReadID(sd, &type));
  }

  for(i = 0; i < *numUnPackStreams; i++)
  {
    (*digestsDefined)[i] = 0;
    (*digests)[i] = 0;
  }


  for(i = 0; i < numFolders; i++)
  {
    UInt32 numSubstreams = folders[i].NumUnPackStreams;
    if (numSubstreams != 1 || !folders[i].UnPackCRCDefined)
      numDigests += numSubstreams;
  }

 
  si = 0;
  while(1)
  {
    if (type == k7zIdCRC)
    {
      int digestIndex = 0;
      Byte *digestsDefined2 = 0; 
      UInt32 *digests2 = 0;
      SZ_RESULT res = SzReadHashDigests(sd, numDigests, &digestsDefined2, &digests2, allocTemp->Alloc);
      if (res == SZ_OK)
      {
        for (i = 0; i < numFolders; i++)
        {
          CFolder *folder = folders + i;
          UInt32 numSubstreams = folder->NumUnPackStreams;
          if (numSubstreams == 1 && folder->UnPackCRCDefined)
          {
            (*digestsDefined)[si] = 1;
            (*digests)[si] = folder->UnPackCRC;
            si++;
          }
          else
          {
            UInt32 j;
            for (j = 0; j < numSubstreams; j++, digestIndex++)
            {
              (*digestsDefined)[si] = digestsDefined2[digestIndex];
              (*digests)[si] = digests2[digestIndex];
              si++;
            }
          }
        }
      }
      allocTemp->Free(digestsDefined2);
      allocTemp->Free(digests2);
      RINOK(res);
    }
    else if (type == k7zIdEnd)
      return SZ_OK;
    else
    {
      RINOK(SzSkeepData(sd));
    }
    RINOK(SzReadID(sd, &type));
  }
}
//////////////////////////////////////////////////////////
SZ_RESULT zip_pack::SzReadStreamsInfo(
    CSzData *sd, 
    CFileSize *dataOffset,
    CArchiveDatabase *db,
    UInt32 *numUnPackStreams,
    CFileSize **unPackSizes, /* allocTemp */
    Byte **digestsDefined,   /* allocTemp */
    UInt32 **digests,        /* allocTemp */
    void * (*allocFunc)(size_t size),
    ISzAlloc *allocTemp)
{
  while(1)
  {
    UInt64 type;
    RINOK(SzReadID(sd, &type));
    if ((UInt64)(int)type != type)
      return SZE_FAIL;
    switch((int)type)
    {
      case k7zIdEnd:
        return SZ_OK;
      case k7zIdPackInfo:
      {
        RINOK(SzReadPackInfo(sd, dataOffset, &db->NumPackStreams, 
            &db->PackSizes, &db->PackCRCsDefined, &db->PackCRCs, allocFunc));
        break;
      }
      case k7zIdUnPackInfo:
      {
        RINOK(SzReadUnPackInfo(sd, &db->NumFolders, &db->Folders, allocFunc, allocTemp));
        break;
      }
      case k7zIdSubStreamsInfo:
      {
        RINOK(SzReadSubStreamsInfo(sd, db->NumFolders, db->Folders, 
            numUnPackStreams, unPackSizes, digestsDefined, digests, allocTemp));
        break;
      }
      default:
        return SZE_FAIL;
    }
  }
}
///////////////////////////////////////////////////////////
SZ_RESULT zip_pack::SzReadFileNames(CSzData *sd, UInt32 numFiles, CFileItem *files, 
    void * (*allocFunc)(size_t size))
{
  UInt32 i;
  for(i = 0; i < numFiles; i++)
  {
    UInt32 len = 0;
    UInt32 pos = 0;
    CFileItem *file = files + i;
    while(pos + 2 <= sd->Size)
    {
      int numAdds;
      UInt32 value = (UInt32)(sd->Data[pos] | (((UInt32)sd->Data[pos + 1]) << 8));
      pos += 2;
      len++;
      if (value == 0)
        break;
      if (value < 0x80)
        continue;
      if (value >= 0xD800 && value < 0xE000)
      {
        UInt32 c2;
        if (value >= 0xDC00)
          return SZE_ARCHIVE_ERROR;
        if (pos + 2 > sd->Size)
          return SZE_ARCHIVE_ERROR;
        c2 = (UInt32)(sd->Data[pos] | (((UInt32)sd->Data[pos + 1]) << 8));
        pos += 2;
        if (c2 < 0xDC00 || c2 >= 0xE000)
          return SZE_ARCHIVE_ERROR;
        value = ((value - 0xD800) << 10) | (c2 - 0xDC00);
      }
      for (numAdds = 1; numAdds < 5; numAdds++)
        if (value < (((UInt32)1) << (numAdds * 5 + 6)))
          break;
      len += numAdds;
    }

    RINOK(MySzInAlloc((void **)&file->Name, (size_t)len * sizeof(char), allocFunc));

    len = 0;
    while(2 <= sd->Size)
    {
      int numAdds;
      UInt32 value = (UInt32)(sd->Data[0] | (((UInt32)sd->Data[1]) << 8));
      SzSkeepDataSize(sd, 2);
      if (value < 0x80)
      {
        file->Name[len++] = (char)value;
        if (value == 0)
          break;
        continue;
      }
      if (value >= 0xD800 && value < 0xE000)
      {
        UInt32 c2 = (UInt32)(sd->Data[0] | (((UInt32)sd->Data[1]) << 8));
        SzSkeepDataSize(sd, 2);
        value = ((value - 0xD800) << 10) | (c2 - 0xDC00);
      }
      for (numAdds = 1; numAdds < 5; numAdds++)
        if (value < (((UInt32)1) << (numAdds * 5 + 6)))
          break;
      file->Name[len++] = (char)(kUtf8Limits[numAdds - 1] + (value >> (6 * numAdds)));
      do
      {
        numAdds--;
        file->Name[len++] = (char)(0x80 + ((value >> (6 * numAdds)) & 0x3F));
      }
      while(numAdds > 0);

      len += numAdds;
    }
  }
  return SZ_OK;
}
///////////////////////////////////////////////////////////////
SZ_RESULT zip_pack::SzReadHeader2(
    CSzData *sd, 
    CArchiveDatabaseEx *db,   /* allocMain */
    CFileSize **unPackSizes,  /* allocTemp */
    Byte **digestsDefined,    /* allocTemp */
    UInt32 **digests,         /* allocTemp */
    Byte **emptyStreamVector, /* allocTemp */
    Byte **emptyFileVector,   /* allocTemp */
    ISzAlloc *allocMain, 
    ISzAlloc *allocTemp)
{
  UInt64 type;
  UInt32 numUnPackStreams = 0;
  UInt32 numFiles = 0;
  CFileItem *files = 0;
  UInt32 numEmptyStreams = 0;
  UInt32 i;

  RINOK(SzReadID(sd, &type));

  if (type == k7zIdArchiveProperties)
  {
    RINOK(SzReadArchiveProperties(sd));
    RINOK(SzReadID(sd, &type));
  }
 
 
  if (type == k7zIdMainStreamsInfo)
  {
    RINOK(SzReadStreamsInfo(sd,
        &db->ArchiveInfo.DataStartPosition,
        &db->Database, 
        &numUnPackStreams,
        unPackSizes,
        digestsDefined,
        digests, allocMain->Alloc, allocTemp));
    db->ArchiveInfo.DataStartPosition += db->ArchiveInfo.StartPositionAfterHeader;
    RINOK(SzReadID(sd, &type));
  }

  if (type == k7zIdEnd)
    return SZ_OK;
  if (type != k7zIdFilesInfo)
    return SZE_ARCHIVE_ERROR;
  
  RINOK(SzReadNumber32(sd, &numFiles));
  db->Database.NumFiles = numFiles;

  RINOK(MySzInAlloc((void **)&files, (size_t)numFiles * sizeof(CFileItem), allocMain->Alloc));

  db->Database.Files = files;
  for(i = 0; i < numFiles; i++)
    SzFileInit(files + i);

  while(1)
  {
    UInt64 type;
    UInt64 size;
    RINOK(SzReadID(sd, &type));
    if (type == k7zIdEnd)
      break;
    RINOK(SzReadNumber(sd, &size));

    if ((UInt64)(int)type != type)
    {
      RINOK(SzSkeepDataSize(sd, size));
    }
    else
    switch((int)type)
    {
      case k7zIdName:
      {
        RINOK(SzReadSwitch(sd));
        RINOK(SzReadFileNames(sd, numFiles, files, allocMain->Alloc))
        break;
      }
      case k7zIdEmptyStream:
      {
        RINOK(SzReadBoolVector(sd, numFiles, emptyStreamVector, allocTemp->Alloc));
        numEmptyStreams = 0;
        for (i = 0; i < numFiles; i++)
          if ((*emptyStreamVector)[i])
            numEmptyStreams++;
        break;
      }
      case k7zIdEmptyFile:
      {
        RINOK(SzReadBoolVector(sd, numEmptyStreams, emptyFileVector, allocTemp->Alloc));
        break;
      }
      default:
      {
        RINOK(SzSkeepDataSize(sd, size));
      }
    }
  }

  {
    UInt32 emptyFileIndex = 0;
    UInt32 sizeIndex = 0;
    for(i = 0; i < numFiles; i++)
    {
      CFileItem *file = files + i;
      file->IsAnti = 0;
      if (*emptyStreamVector == 0)
        file->HasStream = 1;
      else
        file->HasStream = (Byte)((*emptyStreamVector)[i] ? 0 : 1);
      if(file->HasStream)
      {
        file->IsDirectory = 0;
        file->Size = (*unPackSizes)[sizeIndex];
        file->FileCRC = (*digests)[sizeIndex];
        file->IsFileCRCDefined = (Byte)(*digestsDefined)[sizeIndex];
        sizeIndex++;
      }
      else
      {
        if (*emptyFileVector == 0)
          file->IsDirectory = 1;
        else
          file->IsDirectory = (Byte)((*emptyFileVector)[emptyFileIndex] ? 0 : 1);
        emptyFileIndex++;
        file->Size = 0;
        file->IsFileCRCDefined = 0;
      }
    }
  }
  return SzArDbExFill(db, allocMain->Alloc);
}
//////////////////////////////////////////////////////////////
void zip_pack::SzFileInit(CFileItem *fileItem)
{
  fileItem->IsFileCRCDefined = 0;
  fileItem->HasStream = 1;
  fileItem->IsDirectory = 0;
  fileItem->IsAnti = 0;
  fileItem->Name = 0;
}
//////////////////////////////////////////////////////////////
SZ_RESULT zip_pack::SzReadHeader(
    CSzData *sd, 
    CArchiveDatabaseEx *db, 
    ISzAlloc *allocMain, 
    ISzAlloc *allocTemp)
{
  CFileSize *unPackSizes = 0;
  Byte *digestsDefined = 0;
  UInt32 *digests = 0;
  Byte *emptyStreamVector = 0;
  Byte *emptyFileVector = 0;
  SZ_RESULT res = SzReadHeader2(sd, db, 
      &unPackSizes, &digestsDefined, &digests,
      &emptyStreamVector, &emptyFileVector,
      allocMain, allocTemp);
  allocTemp->Free(unPackSizes);
  allocTemp->Free(digestsDefined);
  allocTemp->Free(digests);
  allocTemp->Free(emptyStreamVector);
  allocTemp->Free(emptyFileVector);
  return res;
}
//////////////////////////////////////////////////////////////////
int zip_pack::SzByteBufferCreate(CSzByteBuffer *buffer, size_t newCapacity, void * (*allocFunc)(size_t size))
{
  buffer->Capacity = newCapacity;
  if (newCapacity == 0)
  {
    buffer->Items = 0;
    return 1;
  }
  buffer->Items = (Byte *)allocFunc(newCapacity);
  return (buffer->Items != 0);
}
//////////////////////////////////////////////////////////////////
SZ_RESULT zip_pack::SzDecode(const CFileSize *packSizes, const CFolder *folder,
    #ifdef _LZMA_IN_CB
    ISzInStream *inStream,
    #else
    const Byte *inBuffer,
    #endif
    Byte *outBuffer, size_t outSize, 
    size_t *outSizeProcessed, ISzAlloc *allocMain)
{
  UInt32 si;
  size_t inSize = 0;
  CCoderInfo *coder;
  if (folder->NumPackStreams != 1)
    return SZE_NOTIMPL;
  if (folder->NumCoders != 1)
//  if (folder->NumCoders < 1) - my
    return SZE_NOTIMPL;
  /////////////////////////////// my
//  if (folder->NumCoders != 1)
//	  coder = folder->Coders+1;
//  else
//	  coder = folder->Coders;
  ///////////////////////////////
  coder = folder->Coders;
  *outSizeProcessed = 0;

  for (si = 0; si < folder->NumPackStreams; si++)
    inSize += (size_t)packSizes[si];

  if (AreMethodsEqual(&coder->MethodID, &k_Copy))
  {
    size_t i;
    if (inSize != outSize)
      return SZE_DATA_ERROR;
    #ifdef _LZMA_IN_CB
    for (i = 0; i < inSize;)
    {
      size_t j;
      Byte *inBuffer;
      size_t bufferSize;
      RINOK(inStream->Read((void *)inStream,  (void **)&inBuffer, inSize - i, &bufferSize));
      if (bufferSize == 0)
        return SZE_DATA_ERROR;
      if (bufferSize > inSize - i)
        return SZE_FAIL;
      *outSizeProcessed += bufferSize;
      for (j = 0; j < bufferSize && i < inSize; j++, i++)
        outBuffer[i] = inBuffer[j];
    }
    #else
    for (i = 0; i < inSize; i++)
      outBuffer[i] = inBuffer[i];
    *outSizeProcessed = inSize;
    #endif
    return SZ_OK;
  }

  if (AreMethodsEqual(&coder->MethodID, &k_LZMA))
  {
    #ifdef _LZMA_IN_CB
    CLzmaInCallbackImp lzmaCallback;
    #else
    SizeT inProcessed;
    #endif

    CLzmaDecoderState state;  /* it's about 24-80 bytes structure, if int is 32-bit */
    int result;
    SizeT outSizeProcessedLoc;

    #ifdef _LZMA_IN_CB
    lzmaCallback.Size = inSize;
    lzmaCallback.InStream = inStream;
    lzmaCallback.InCallback.Read = LzmaReadImp;
    #endif

    if (LzmaDecodeProperties(&state.Properties, coder->Properties.Items, 
        coder->Properties.Capacity) != LZMA_RESULT_OK)
      return SZE_FAIL;

    state.Probs = (CProb *)allocMain->Alloc(LzmaGetNumProbs(&state.Properties) * sizeof(CProb));
    if (state.Probs == 0)
      return SZE_OUTOFMEMORY;

    #ifdef _LZMA_OUT_READ
    if (state.Properties.DictionarySize == 0)
      state.Dictionary = 0;
    else
    {
      state.Dictionary = (unsigned char *)allocMain->Alloc(state.Properties.DictionarySize);
      if (state.Dictionary == 0)
      {
        allocMain->Free(state.Probs);
        return SZE_OUTOFMEMORY;
      }
    }
    LzmaDecoderInit(&state);
    #endif

    result = LzmaDecode(&state,
        #ifdef _LZMA_IN_CB
        &lzmaCallback.InCallback,
        #else
        inBuffer, (SizeT)inSize, &inProcessed,
        #endif
        outBuffer, (SizeT)outSize, &outSizeProcessedLoc);
    *outSizeProcessed = (size_t)outSizeProcessedLoc;
    allocMain->Free(state.Probs);
    #ifdef _LZMA_OUT_READ
    allocMain->Free(state.Dictionary);
    #endif
    if (result == LZMA_RESULT_DATA_ERROR)
      return SZE_DATA_ERROR;
    if (result != LZMA_RESULT_OK)
      return SZE_FAIL;
    return SZ_OK;
  }
  return SZE_NOTIMPL;
}
////////////////////////////////////////////////////////////
int zip_pack::AreMethodsEqual(CMethodID *a1, CMethodID *a2)
{
  int i;
  if (a1->IDSize != a2->IDSize)
    return 0;
  for (i = 0; i < a1->IDSize; i++)
    if (a1->ID[i] != a2->ID[i])
      return 0;
  return 1;
} 
////////////////////////////////////////////////////////////
SZ_RESULT zip_pack::MySzInAlloc(void **p, size_t size, void * (*allocFunc)(size_t size))
{
  if (size == 0)
    *p = 0;
  else
  {
    *p = allocFunc(size);
    RINOM(*p);
  }
  return SZ_OK;
}
////////////////////////////////////////////////////////////
void zip_pack::SzArDbExInit(CArchiveDatabaseEx *db)
{
  SzArchiveDatabaseInit(&db->Database);
  db->FolderStartPackStreamIndex = 0;
  db->PackStreamStartPositions = 0;
  db->FolderStartFileIndex = 0;
  db->FileIndexToFolderIndexMap = 0;
}
//////////////////////////////////////////////////////////////
void zip_pack::SzArDbExFree(CArchiveDatabaseEx *db, void (*freeFunc)(void *))
{
  freeFunc(db->FolderStartPackStreamIndex);
  freeFunc(db->PackStreamStartPositions);
  freeFunc(db->FolderStartFileIndex);
  freeFunc(db->FileIndexToFolderIndexMap);
  SzArchiveDatabaseFree(&db->Database, freeFunc);
  SzArDbExInit(db);
}
/////////////////////////////////////////////////////////////
SZ_RESULT zip_pack::SzArDbExFill(CArchiveDatabaseEx *db, void * (*allocFunc)(size_t size))
{
  UInt32 startPos = 0;
  CFileSize startPosSize = 0;
  UInt32 i;
  UInt32 folderIndex = 0;
  UInt32 indexInFolder = 0;
  RINOK(MySzInAlloc((void **)&db->FolderStartPackStreamIndex, db->Database.NumFolders * sizeof(UInt32), allocFunc));
  for(i = 0; i < db->Database.NumFolders; i++)
  {
    db->FolderStartPackStreamIndex[i] = startPos;
    startPos += db->Database.Folders[i].NumPackStreams;
  }

  RINOK(MySzInAlloc((void **)&db->PackStreamStartPositions, db->Database.NumPackStreams * sizeof(CFileSize), allocFunc));

  for(i = 0; i < db->Database.NumPackStreams; i++)
  {
    db->PackStreamStartPositions[i] = startPosSize;
    startPosSize += db->Database.PackSizes[i];
  }

  RINOK(MySzInAlloc((void **)&db->FolderStartFileIndex, db->Database.NumFolders * sizeof(UInt32), allocFunc));
  RINOK(MySzInAlloc((void **)&db->FileIndexToFolderIndexMap, db->Database.NumFiles * sizeof(UInt32), allocFunc));

  for (i = 0; i < db->Database.NumFiles; i++)
  {
    CFileItem *file = db->Database.Files + i;
    int emptyStream = !file->HasStream;
    if (emptyStream && indexInFolder == 0)
    {
      db->FileIndexToFolderIndexMap[i] = (UInt32)-1;
      continue;
    }
    if (indexInFolder == 0)
    {
      /*
      v3.13 incorrectly worked with empty folders
      v4.07: Loop for skipping empty folders
      */
      while(1)
      {
        if (folderIndex >= db->Database.NumFolders)
          return SZE_ARCHIVE_ERROR;
        db->FolderStartFileIndex[folderIndex] = i;
        if (db->Database.Folders[folderIndex].NumUnPackStreams != 0)
          break;
        folderIndex++;
      }
    }
    db->FileIndexToFolderIndexMap[i] = folderIndex;
    if (emptyStream)
      continue;
    indexInFolder++;
    if (indexInFolder >= db->Database.Folders[folderIndex].NumUnPackStreams)
    {
      folderIndex++;
      indexInFolder = 0;
    }
  }
  return SZ_OK;
}
////////////////////////////////////////////////////////
void zip_pack::SzArchiveDatabaseInit(CArchiveDatabase *db)
{
  db->NumPackStreams = 0;
  db->PackSizes = 0;
  db->PackCRCsDefined = 0;
  db->PackCRCs = 0;
  db->NumFolders = 0;
  db->Folders = 0;
  db->NumFiles = 0;
  db->Files = 0;
}
////////////////////////////////////////////////////////
void zip_pack::SzArchiveDatabaseFree(CArchiveDatabase *db, void (*freeFunc)(void *))
{
  UInt32 i;
  for (i = 0; i < db->NumFolders; i++)
    SzFolderFree(&db->Folders[i], freeFunc);
  for (i = 0; i < db->NumFiles; i++)
    SzFileFree(&db->Files[i], freeFunc);
  freeFunc(db->PackSizes);
  freeFunc(db->PackCRCsDefined);
  freeFunc(db->PackCRCs);
  freeFunc(db->Folders);
  freeFunc(db->Files);
  SzArchiveDatabaseInit(db);
}
////////////////////////////////////////////////////////
void zip_pack::SzFileFree(CFileItem *fileItem, void (*freeFunc)(void *p))
{
  freeFunc(fileItem->Name);
  SzFileInit(fileItem);
}
////////////////////////////////////////////////////////
void zip_pack::SzFolderFree(CFolder *folder, void (*freeFunc)(void *p))
{
  UInt32 i;
  for (i = 0; i < folder->NumCoders; i++)
    SzCoderInfoFree(&folder->Coders[i], freeFunc);
  freeFunc(folder->Coders);
  freeFunc(folder->BindPairs);
  freeFunc(folder->PackStreams);
  freeFunc(folder->UnPackSizes);
  SzFolderInit(folder);
}
////////////////////////////////////////////////////////
void zip_pack::SzCoderInfoFree(CCoderInfo *coder, void (*freeFunc)(void *p))
{
  SzByteBufferFree(&coder->Properties, freeFunc);
  SzCoderInfoInit(coder);
}
////////////////////////////////////////////////////////
CFileSize zip_pack::SzArDbGetFolderStreamPos(CArchiveDatabaseEx *db, UInt32 folderIndex, UInt32 indexInFolder)
{
  return db->ArchiveInfo.DataStartPosition + 
    db->PackStreamStartPositions[db->FolderStartPackStreamIndex[folderIndex] + indexInFolder];
}
////////////////////////////////////////////////////////
CFileSize zip_pack::SzArDbGetFolderFullPackSize(CArchiveDatabaseEx *db, UInt32 folderIndex)
{
  UInt32 packStreamIndex = db->FolderStartPackStreamIndex[folderIndex];
  CFolder *folder = db->Database.Folders + folderIndex;
  CFileSize size = 0;
  UInt32 i;
  for (i = 0; i < folder->NumPackStreams; i++)
    size += db->Database.PackSizes[packStreamIndex + i];
  return size;
}
/////////////////////////////////////////////////////////
int zip_pack::TestSignatureCandidate(Byte *testBytes)
{
  size_t i;
  for (i = 0; i < k7zSignatureSize; i++)
    if (testBytes[i] != k7zSignature[i])
      return 0;
  return 1;
}
/////////////////////////////////////////////////////////
CFileSize zip_pack::SzFolderGetUnPackSize(CFolder *folder)
{ 
  int i = (int)SzFolderGetNumOutStreams(folder);
  if (i == 0)
    return 0;
  for (i--; i >= 0; i--)
    if (SzFolderFindBindPairForOutStream(folder, i) < 0)
      return folder->UnPackSizes[i];
  /* throw 1; */
  return 0;
}
/////////////////////////////////////////////////////////
int zip_pack::SzFolderFindBindPairForOutStream(CFolder *folder, UInt32 outStreamIndex)
{
  UInt32 i;
  for(i = 0; i < folder->NumBindPairs; i++)
    if (folder->BindPairs[i].OutIndex == outStreamIndex)
      return i;
  return -1;
} 
/////////////////////////////////////////////////////////
void zip_pack::InitCrcTable()
{
  UInt32 i;
  for (i = 0; i < 256; i++)
  {
    UInt32 r = i;
    int j;
    for (j = 0; j < 8; j++)
      if (r & 1) 
        r = (r >> 1) ^ kCrcPoly;
      else     
        r >>= 1;
    g_CrcTable[i] = r;
  }
}
/////////////////////////////////////////////////////////
int zip_pack::CrcVerifyDigest(UInt32 digest, const void *data, size_t size)
{
  return (CrcCalculateDigest(data, size) == digest);
}
/////////////////////////////////////////////////////////
UInt32 zip_pack::CrcCalculateDigest(const void *data, size_t size)
{
  UInt32 crc;
  CrcInit(&crc);
  CrcUpdate(&crc, data, size);
  return CrcGetDigest(&crc);
}
/////////////////////////////////////////////////////////
void zip_pack::CrcUpdate(UInt32 *crc, const void *data, size_t size)
{
  UInt32 v = *crc;
  const Byte *p = (const Byte *)data;
  for (; size > 0 ; size--, p++)
    v = g_CrcTable[((Byte)(v)) ^ *p] ^ (v >> 8);
  *crc = v;
}
/////////////////////////////////////////////////////////
void *SzAlloc(size_t size)
{
  if (size == 0)
    return 0;
  #ifdef _SZ_ALLOC_DEBUG
  fprintf(stderr, "\nAlloc %10d bytes; count = %10d", size, g_allocCount);
  g_allocCount++;
  #endif
  return malloc(size);
}

void SzFree(void *address)
{
  free(address);
}

void *SzAllocTemp(size_t size)
{
  if (size == 0)
    return 0;
  #ifdef _SZ_ALLOC_DEBUG
  fprintf(stderr, "\nAlloc_temp %10d bytes;  count = %10d", size, g_allocCountTemp);
  g_allocCountTemp++;
  #ifdef _WIN32
  return HeapAlloc(GetProcessHeap(), 0, size);
  #endif
  #endif
  return malloc(size);
}

void SzFreeTemp(void *address)
{
  #ifdef _SZ_ALLOC_DEBUG
  if (address != 0)
  {
    g_allocCountTemp--;
    fprintf(stderr, "\nFree_temp; count = %10d", g_allocCountTemp);
  }
  #ifdef _WIN32
  HeapFree(GetProcessHeap(), 0, address);
  return;
  #endif
  #endif
  free(address);
}
///////////////////////////////////////////////////////////////

#ifndef Byte
#define Byte unsigned char
#endif

#define kNumTopBits 24
#define kTopValue ((UInt32)1 << kNumTopBits)

#define kNumBitModelTotalBits 11
#define kBitModelTotal (1 << kNumBitModelTotalBits)
#define kNumMoveBits 5

#define RC_READ_BYTE (*Buffer++)

#define RC_INIT2 Code = 0; Range = 0xFFFFFFFF; \
  { int i; for(i = 0; i < 5; i++) { RC_TEST; Code = (Code << 8) | RC_READ_BYTE; }}

#ifdef _LZMA_IN_CB

#define RC_TEST { if (Buffer == BufferLim) \
  { SizeT size; int result = InCallback->Read(InCallback, &Buffer, &size); if (result != LZMA_RESULT_OK) return result; \
  BufferLim = Buffer + size; if (size == 0) return LZMA_RESULT_DATA_ERROR; }}

#define RC_INIT Buffer = BufferLim = 0; RC_INIT2

#else

#define RC_TEST { if (Buffer == BufferLim) return LZMA_RESULT_DATA_ERROR; }

#define RC_INIT(buffer, bufferSize) Buffer = buffer; BufferLim = buffer + bufferSize; RC_INIT2
 
#endif

#define RC_NORMALIZE if (Range < kTopValue) { RC_TEST; Range <<= 8; Code = (Code << 8) | RC_READ_BYTE; }

#define IfBit0(p) RC_NORMALIZE; bound = (Range >> kNumBitModelTotalBits) * *(p); if (Code < bound)
#define UpdateBit0(p) Range = bound; *(p) += (kBitModelTotal - *(p)) >> kNumMoveBits;
#define UpdateBit1(p) Range -= bound; Code -= bound; *(p) -= (*(p)) >> kNumMoveBits;

#define RC_GET_BIT2(p, mi, A0, A1) IfBit0(p) \
  { UpdateBit0(p); mi <<= 1; A0; } else \
  { UpdateBit1(p); mi = (mi + mi) + 1; A1; } 
  
#define RC_GET_BIT(p, mi) RC_GET_BIT2(p, mi, ; , ;)               

#define RangeDecoderBitTreeDecode(probs, numLevels, res) \
  { int i = numLevels; res = 1; \
  do { CProb *p = probs + res; RC_GET_BIT(p, res) } while(--i != 0); \
  res -= (1 << numLevels); }


#define kNumPosBitsMax 4
#define kNumPosStatesMax (1 << kNumPosBitsMax)

#define kLenNumLowBits 3
#define kLenNumLowSymbols (1 << kLenNumLowBits)
#define kLenNumMidBits 3
#define kLenNumMidSymbols (1 << kLenNumMidBits)
#define kLenNumHighBits 8
#define kLenNumHighSymbols (1 << kLenNumHighBits)

#define LenChoice 0
#define LenChoice2 (LenChoice + 1)
#define LenLow (LenChoice2 + 1)
#define LenMid (LenLow + (kNumPosStatesMax << kLenNumLowBits))
#define LenHigh (LenMid + (kNumPosStatesMax << kLenNumMidBits))
#define kNumLenProbs (LenHigh + kLenNumHighSymbols) 


#define kNumStates 12
#define kNumLitStates 7

#define kStartPosModelIndex 4
#define kEndPosModelIndex 14
#define kNumFullDistances (1 << (kEndPosModelIndex >> 1))

#define kNumPosSlotBits 6
#define kNumLenToPosStates 4

#define kNumAlignBits 4
#define kAlignTableSize (1 << kNumAlignBits)

#define kMatchMinLen 2

#define IsMatch 0
#define IsRep (IsMatch + (kNumStates << kNumPosBitsMax))
#define IsRepG0 (IsRep + kNumStates)
#define IsRepG1 (IsRepG0 + kNumStates)
#define IsRepG2 (IsRepG1 + kNumStates)
#define IsRep0Long (IsRepG2 + kNumStates)
#define PosSlot (IsRep0Long + (kNumStates << kNumPosBitsMax))
#define SpecPos (PosSlot + (kNumLenToPosStates << kNumPosSlotBits))
#define Align (SpecPos + kNumFullDistances - kEndPosModelIndex)
#define LenCoder (Align + kAlignTableSize)
#define RepLenCoder (LenCoder + kNumLenProbs)
#define Literal (RepLenCoder + kNumLenProbs)

#if Literal != LZMA_BASE_SIZE
StopCompilingDueBUG
#endif
#define kLzmaStreamWasFinishedId (-1)

int zip_pack::LzmaDecodeProperties(CLzmaProperties *propsRes, const unsigned char *propsData, int size)
{
  unsigned char prop0;
  if (size < LZMA_PROPERTIES_SIZE)
    return LZMA_RESULT_DATA_ERROR;
  prop0 = propsData[0];
  if (prop0 >= (9 * 5 * 5))
    return LZMA_RESULT_DATA_ERROR;
  {
    for (propsRes->pb = 0; prop0 >= (9 * 5); propsRes->pb++, prop0 -= (9 * 5));
    for (propsRes->lp = 0; prop0 >= 9; propsRes->lp++, prop0 -= 9);
    propsRes->lc = prop0;
    /*
    unsigned char remainder = (unsigned char)(prop0 / 9);
    propsRes->lc = prop0 % 9;
    propsRes->pb = remainder / 5;
    propsRes->lp = remainder % 5;
    */
  }

  #ifdef _LZMA_OUT_READ
  {
    int i;
    propsRes->DictionarySize = 0;
    for (i = 0; i < 4; i++)
      propsRes->DictionarySize += (UInt32)(propsData[1 + i]) << (i * 8);
    if (propsRes->DictionarySize == 0)
      propsRes->DictionarySize = 1;
  }
  #endif
  return LZMA_RESULT_OK;
}
int zip_pack::LzmaDecode(CLzmaDecoderState *vs,
    #ifdef _LZMA_IN_CB
    ILzmaInCallback *InCallback,
    #else
    const unsigned char *inStream, SizeT inSize, SizeT *inSizeProcessed,
    #endif
    unsigned char *outStream, SizeT outSize, SizeT *outSizeProcessed)
{
  CProb *p = vs->Probs;
  SizeT nowPos = 0;
  Byte previousByte = 0;
  UInt32 posStateMask = (1 << (vs->Properties.pb)) - 1;
  UInt32 literalPosMask = (1 << (vs->Properties.lp)) - 1;
  int lc = vs->Properties.lc;

  #ifdef _LZMA_OUT_READ
  
  UInt32 Range = vs->Range;
  UInt32 Code = vs->Code;
  #ifdef _LZMA_IN_CB
  const Byte *Buffer = vs->Buffer;
  const Byte *BufferLim = vs->BufferLim;
  #else
  const Byte *Buffer = inStream;
  const Byte *BufferLim = inStream + inSize;
  #endif
  int state = vs->State;
  UInt32 rep0 = vs->Reps[0], rep1 = vs->Reps[1], rep2 = vs->Reps[2], rep3 = vs->Reps[3];
  int len = vs->RemainLen;
  UInt32 globalPos = vs->GlobalPos;
  UInt32 distanceLimit = vs->DistanceLimit;

  Byte *dictionary = vs->Dictionary;
  UInt32 dictionarySize = vs->Properties.DictionarySize;
  UInt32 dictionaryPos = vs->DictionaryPos;

  Byte tempDictionary[4];

  #ifndef _LZMA_IN_CB
  *inSizeProcessed = 0;
  #endif
  *outSizeProcessed = 0;
  if (len == kLzmaStreamWasFinishedId)
    return LZMA_RESULT_OK;

  if (dictionarySize == 0)
  {
    dictionary = tempDictionary;
    dictionarySize = 1;
    tempDictionary[0] = vs->TempDictionary[0];
  }

  if (len == kLzmaNeedInitId)
  {
    {
      UInt32 numProbs = Literal + ((UInt32)LZMA_LIT_SIZE << (lc + vs->Properties.lp));
      UInt32 i;
      for (i = 0; i < numProbs; i++)
        p[i] = kBitModelTotal >> 1; 
      rep0 = rep1 = rep2 = rep3 = 1;
      state = 0;
      globalPos = 0;
      distanceLimit = 0;
      dictionaryPos = 0;
      dictionary[dictionarySize - 1] = 0;
      #ifdef _LZMA_IN_CB
      RC_INIT;
      #else
      RC_INIT(inStream, inSize);
      #endif
    }
    len = 0;
  }
  while(len != 0 && nowPos < outSize)
  {
    UInt32 pos = dictionaryPos - rep0;
    if (pos >= dictionarySize)
      pos += dictionarySize;
    outStream[nowPos++] = dictionary[dictionaryPos] = dictionary[pos];
    if (++dictionaryPos == dictionarySize)
      dictionaryPos = 0;
    len--;
  }
  if (dictionaryPos == 0)
    previousByte = dictionary[dictionarySize - 1];
  else
    previousByte = dictionary[dictionaryPos - 1];

  #else /* if !_LZMA_OUT_READ */

  int state = 0;
  UInt32 rep0 = 1, rep1 = 1, rep2 = 1, rep3 = 1;
  int len = 0;
  const Byte *Buffer;
  const Byte *BufferLim;
  UInt32 Range;
  UInt32 Code;

  #ifndef _LZMA_IN_CB
  *inSizeProcessed = 0;
  #endif
  *outSizeProcessed = 0;

  {
    UInt32 i;
    UInt32 numProbs = Literal + ((UInt32)LZMA_LIT_SIZE << (lc + vs->Properties.lp));
    for (i = 0; i < numProbs; i++)
      p[i] = kBitModelTotal >> 1;
  }
  
  #ifdef _LZMA_IN_CB
  RC_INIT;
  #else
  RC_INIT(inStream, inSize);
  #endif

  #endif /* _LZMA_OUT_READ */

  while(nowPos < outSize)
  {
    CProb *prob;
    UInt32 bound;
    int posState = (int)(
        (nowPos 
        #ifdef _LZMA_OUT_READ
        + globalPos
        #endif
        )
        & posStateMask);

    prob = p + IsMatch + (state << kNumPosBitsMax) + posState;
    IfBit0(prob)
    {
      int symbol = 1;
      UpdateBit0(prob)
      prob = p + Literal + (LZMA_LIT_SIZE * 
        (((
        (nowPos 
        #ifdef _LZMA_OUT_READ
        + globalPos
        #endif
        )
        & literalPosMask) << lc) + (previousByte >> (8 - lc))));

      if (state >= kNumLitStates)
      {
        int matchByte;
        #ifdef _LZMA_OUT_READ
        UInt32 pos = dictionaryPos - rep0;
        if (pos >= dictionarySize)
          pos += dictionarySize;
        matchByte = dictionary[pos];
        #else
        matchByte = outStream[nowPos - rep0];
        #endif
        do
        {
          int bit;
          CProb *probLit;
          matchByte <<= 1;
          bit = (matchByte & 0x100);
          probLit = prob + 0x100 + bit + symbol;
          RC_GET_BIT2(probLit, symbol, if (bit != 0) break, if (bit == 0) break)
        }
        while (symbol < 0x100);
      }
      while (symbol < 0x100)
      {
        CProb *probLit = prob + symbol;
        RC_GET_BIT(probLit, symbol)
      }
      previousByte = (Byte)symbol;

      outStream[nowPos++] = previousByte;
      #ifdef _LZMA_OUT_READ
      if (distanceLimit < dictionarySize)
        distanceLimit++;

      dictionary[dictionaryPos] = previousByte;
      if (++dictionaryPos == dictionarySize)
        dictionaryPos = 0;
      #endif
      if (state < 4) state = 0;
      else if (state < 10) state -= 3;
      else state -= 6;
    }
    else             
    {
      UpdateBit1(prob);
      prob = p + IsRep + state;
      IfBit0(prob)
      {
        UpdateBit0(prob);
        rep3 = rep2;
        rep2 = rep1;
        rep1 = rep0;
        state = state < kNumLitStates ? 0 : 3;
        prob = p + LenCoder;
      }
      else
      {
        UpdateBit1(prob);
        prob = p + IsRepG0 + state;
        IfBit0(prob)
        {
          UpdateBit0(prob);
          prob = p + IsRep0Long + (state << kNumPosBitsMax) + posState;
          IfBit0(prob)
          {
            #ifdef _LZMA_OUT_READ
            UInt32 pos;
            #endif
            UpdateBit0(prob);
            
            #ifdef _LZMA_OUT_READ
            if (distanceLimit == 0)
            #else
            if (nowPos == 0)
            #endif
              return LZMA_RESULT_DATA_ERROR;
            
            state = state < kNumLitStates ? 9 : 11;
            #ifdef _LZMA_OUT_READ
            pos = dictionaryPos - rep0;
            if (pos >= dictionarySize)
              pos += dictionarySize;
            previousByte = dictionary[pos];
            dictionary[dictionaryPos] = previousByte;
            if (++dictionaryPos == dictionarySize)
              dictionaryPos = 0;
            #else
            previousByte = outStream[nowPos - rep0];
            #endif
            outStream[nowPos++] = previousByte;
            #ifdef _LZMA_OUT_READ
            if (distanceLimit < dictionarySize)
              distanceLimit++;
            #endif

            continue;
          }
          else
          {
            UpdateBit1(prob);
          }
        }
        else
        {
          UInt32 distance;
          UpdateBit1(prob);
          prob = p + IsRepG1 + state;
          IfBit0(prob)
          {
            UpdateBit0(prob);
            distance = rep1;
          }
          else 
          {
            UpdateBit1(prob);
            prob = p + IsRepG2 + state;
            IfBit0(prob)
            {
              UpdateBit0(prob);
              distance = rep2;
            }
            else
            {
              UpdateBit1(prob);
              distance = rep3;
              rep3 = rep2;
            }
            rep2 = rep1;
          }
          rep1 = rep0;
          rep0 = distance;
        }
        state = state < kNumLitStates ? 8 : 11;
        prob = p + RepLenCoder;
      }
      {
        int numBits, offset;
        CProb *probLen = prob + LenChoice;
        IfBit0(probLen)
        {
          UpdateBit0(probLen);
          probLen = prob + LenLow + (posState << kLenNumLowBits);
          offset = 0;
          numBits = kLenNumLowBits;
        }
        else
        {
          UpdateBit1(probLen);
          probLen = prob + LenChoice2;
          IfBit0(probLen)
          {
            UpdateBit0(probLen);
            probLen = prob + LenMid + (posState << kLenNumMidBits);
            offset = kLenNumLowSymbols;
            numBits = kLenNumMidBits;
          }
          else
          {
            UpdateBit1(probLen);
            probLen = prob + LenHigh;
            offset = kLenNumLowSymbols + kLenNumMidSymbols;
            numBits = kLenNumHighBits;
          }
        }
        RangeDecoderBitTreeDecode(probLen, numBits, len);
        len += offset;
      }

      if (state < 4)
      {
        int posSlot;
        state += kNumLitStates;
        prob = p + PosSlot +
            ((len < kNumLenToPosStates ? len : kNumLenToPosStates - 1) << 
            kNumPosSlotBits);
        RangeDecoderBitTreeDecode(prob, kNumPosSlotBits, posSlot);
        if (posSlot >= kStartPosModelIndex)
        {
          int numDirectBits = ((posSlot >> 1) - 1);
          rep0 = (2 | ((UInt32)posSlot & 1));
          if (posSlot < kEndPosModelIndex)
          {
            rep0 <<= numDirectBits;
            prob = p + SpecPos + rep0 - posSlot - 1;
          }
          else
          {
            numDirectBits -= kNumAlignBits;
            do
            {
              RC_NORMALIZE
              Range >>= 1;
              rep0 <<= 1;
              if (Code >= Range)
              {
                Code -= Range;
                rep0 |= 1;
              }
            }
            while (--numDirectBits != 0);
            prob = p + Align;
            rep0 <<= kNumAlignBits;
            numDirectBits = kNumAlignBits;
          }
          {
            int i = 1;
            int mi = 1;
            do
            {
              CProb *prob3 = prob + mi;
              RC_GET_BIT2(prob3, mi, ; , rep0 |= i);
              i <<= 1;
            }
            while(--numDirectBits != 0);
          }
        }
        else
          rep0 = posSlot;
        if (++rep0 == (UInt32)(0))
        {
          /* it's for stream version */
          len = kLzmaStreamWasFinishedId;
          break;
        }
      }

      len += kMatchMinLen;
      #ifdef _LZMA_OUT_READ
      if (rep0 > distanceLimit) 
      #else
      if (rep0 > nowPos)
      #endif
        return LZMA_RESULT_DATA_ERROR;

      #ifdef _LZMA_OUT_READ
      if (dictionarySize - distanceLimit > (UInt32)len)
        distanceLimit += len;
      else
        distanceLimit = dictionarySize;
      #endif

      do
      {
        #ifdef _LZMA_OUT_READ
        UInt32 pos = dictionaryPos - rep0;
        if (pos >= dictionarySize)
          pos += dictionarySize;
        previousByte = dictionary[pos];
        dictionary[dictionaryPos] = previousByte;
        if (++dictionaryPos == dictionarySize)
          dictionaryPos = 0;
        #else
        previousByte = outStream[nowPos - rep0];
        #endif
        len--;
        outStream[nowPos++] = previousByte;
      }
      while(len != 0 && nowPos < outSize);
    }
  }
  RC_NORMALIZE;

  #ifdef _LZMA_OUT_READ
  vs->Range = Range;
  vs->Code = Code;
  vs->DictionaryPos = dictionaryPos;
  vs->GlobalPos = globalPos + (UInt32)nowPos;
  vs->DistanceLimit = distanceLimit;
  vs->Reps[0] = rep0;
  vs->Reps[1] = rep1;
  vs->Reps[2] = rep2;
  vs->Reps[3] = rep3;
  vs->State = state;
  vs->RemainLen = len;
  vs->TempDictionary[0] = tempDictionary[0];
  #endif

  #ifdef _LZMA_IN_CB
  vs->Buffer = Buffer;
  vs->BufferLim = BufferLim;
  #else
  *inSizeProcessed = (SizeT)(Buffer - inStream);
  #endif
  *outSizeProcessed = nowPos;
  return LZMA_RESULT_OK;
}
