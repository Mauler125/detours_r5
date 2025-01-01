#include "tier0/binstream.h"
#include <sys/stat.h>

//-----------------------------------------------------------------------------
// Purpose: CIOStream constructors
//-----------------------------------------------------------------------------
CIOStream::CIOStream()
{
	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: CIOStream destructor
//-----------------------------------------------------------------------------
CIOStream::~CIOStream()
{
	Close();
}

//-----------------------------------------------------------------------------
// Purpose: get internal stream mode from selected mode
//-----------------------------------------------------------------------------
static std::ios_base::openmode GetInternalStreamMode(const CIOStream::Mode_e mode)
{
	switch (mode)
	{
	case CIOStream::Mode_e::Read:
		return (std::ios::in | std::ios::binary);
	case CIOStream::Mode_e::Write:
		return (std::ios::out | std::ios::binary);
	case CIOStream::Mode_e::ReadWrite:
		return (std::ios::in | std::ios::out | std::ios::binary);
	case CIOStream::Mode_e::ReadWriteCreate:
		return (std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);
	}

	assert(0); // code bug, can never reach this.
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: opens the file in specified mode
// Input  : *filePath - 
//			mode - 
// Output : true if operation is successful
//-----------------------------------------------------------------------------
bool CIOStream::Open(const char* const filePath, const Mode_e mode)
{
	m_flags = GetInternalStreamMode(mode);
	m_mode = mode;

	if (m_stream.is_open())
	{
		m_stream.close();
	}

	m_stream.open(filePath, GetInternalStreamMode(mode));

	if (!m_stream.is_open() || !m_stream.good())
	{
		return false;
	}

	if (IsReadMode())
	{
		struct _stat64 status;
		if (_stat64(filePath, &status) != NULL)
		{
			return false;
		}

		m_size = status.st_size;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: resets the state
//-----------------------------------------------------------------------------
void CIOStream::Reset()
{
	m_size = 0;
	m_skip = 0;
	m_mode = Mode_e::None;
	m_flags = 0;
}

//-----------------------------------------------------------------------------
// Purpose: closes the stream
//-----------------------------------------------------------------------------
void CIOStream::Close()
{
	m_stream.close();
	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: flushes the ofstream
//-----------------------------------------------------------------------------
void CIOStream::Flush()
{
	if (IsWritable())
		m_stream.flush();
}

//-----------------------------------------------------------------------------
// Purpose: gets the position of the current character in the stream
// Output : std::streampos
//-----------------------------------------------------------------------------
std::streamoff CIOStream::TellGet()
{
	assert(IsReadMode());
	return m_stream.tellg();
}
std::streamoff CIOStream::TellPut()
{
	assert(IsWriteMode());
	return m_stream.tellp();
}

//-----------------------------------------------------------------------------
// Purpose: sets the position of the current character in the stream
// Input  : offset - 
//			way - 
//-----------------------------------------------------------------------------
void CIOStream::SeekGet(const std::streamoff offset, const std::ios_base::seekdir way)
{
	assert(IsReadMode());
	m_stream.seekg(offset, way);
}
//-----------------------------------------------------------------------------
// NOTE: if you seek beyond the end of the file to try and pad it out, use the
// Pad() method instead as the behavior of seek is operating system dependent
//-----------------------------------------------------------------------------
void CIOStream::SeekPut(const std::streamoff offset, const std::ios_base::seekdir way)
{
	assert(IsWriteMode());

	CalcSkipDelta(offset, way);
	m_stream.seekp(offset, way);
}
void CIOStream::Seek(const std::streamoff offset, const std::ios_base::seekdir way)
{
	if (IsReadMode())
		SeekGet(offset, way);
	if (IsWriteMode())
		SeekPut(offset, way);
}

//-----------------------------------------------------------------------------
// Purpose: returns the data
// Output : std::filebuf*
//-----------------------------------------------------------------------------
const std::filebuf* CIOStream::GetData() const
{
	return m_stream.rdbuf();
}

//-----------------------------------------------------------------------------
// Purpose: returns the data size
// Output : std::streampos
//-----------------------------------------------------------------------------
const std::streamoff CIOStream::GetSize() const
{
	return m_size;
}

bool CIOStream::IsReadMode() const
{
	return (m_flags & std::ios::in);
}

bool CIOStream::IsWriteMode() const
{
	return (m_flags & std::ios::out);
}

//-----------------------------------------------------------------------------
// Purpose: checks if we are able to read the file
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool CIOStream::IsReadable() const
{
	if (!IsReadMode() || !m_stream || m_stream.eof())
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: checks if we are able to write to file
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool CIOStream::IsWritable() const
{
	if (!IsWriteMode() || !m_stream)
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: checks if we hit the end of file
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool CIOStream::IsEof() const
{
	return m_stream.eof();
}

//-----------------------------------------------------------------------------
// Purpose: reads a string from the file
// Input  : &svOut - 
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool CIOStream::ReadString(std::string& out)
{
	if (!IsReadable())
		return false;

	while (!m_stream.eof())
	{
		const char c = Read<char>();

		if (c == '\0')
			break;

		out += c;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: reads a string from the file into a fixed size buffer
// Input  : *buf - 
//			len - 
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool CIOStream::ReadString(char* const buf, const size_t len)
{
	if (!IsReadable())
		return false;

	size_t i = 0;

	while (i < len && !m_stream.eof())
	{
		const char c = Read<char>();

		if (c == '\0')
			break;

		buf[i++] = c;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: writes a string to the file
// Input  : &input - 
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool CIOStream::WriteString(const std::string& input, const bool nullterminate)
{
	if (!IsWritable())
		return false;

	const char* const text = input.c_str();
	const size_t len = input.length() + nullterminate;

	m_stream.write(text, len);
	CalcAddDelta(len);

	return true;
}

// limit number of io calls and allocations by just using this static buffer
// for padding out the stream.
static constexpr size_t PAD_BUF_SIZE = 4096;
const static char s_padBuf[PAD_BUF_SIZE];

//-----------------------------------------------------------------------------
// Purpose: pads the out stream up to count bytes
// Input  : count - 
//-----------------------------------------------------------------------------
void CIOStream::Pad(const size_t count)
{
	assert(count > 0);
	size_t remainder = count;

	while (remainder)
	{
		const size_t writeCount = (std::min)(count, PAD_BUF_SIZE);
		Write(s_padBuf, writeCount);

		remainder -= writeCount;
	}
}

//-----------------------------------------------------------------------------
// Purpose: makes sure that the size gets incremented if we exceeded the end of
//          the stream with the delta amount
//-----------------------------------------------------------------------------
void CIOStream::CalcAddDelta(const size_t count)
{
	if (m_skip > 0)
	{
		m_skip -= count;

		if (m_skip < 0)
		{
			m_size += -m_skip; // Add the overshoot to the file size.
			m_skip = 0;
		}
	}
	else
		m_size += count;
}

//-----------------------------------------------------------------------------
// Purpose: if we seek backwards, and then write new data, we should not add
//          this to the total output size of the stream as we modify and not
//          add. we have to keep by how much we shifted backwards and advanced
//          forward until we can start adding again.
//-----------------------------------------------------------------------------
void CIOStream::CalcSkipDelta(const std::streamoff offset, const std::ios_base::seekdir way)
{
	switch (way)
	{
	case std::ios_base::beg:
	{
		if (offset < 0)
		{
			assert(false && "Negative offset in std::ios_base::beg is invalid.");
			return;
		}

		if (offset > m_size)
		{
			m_size = offset;
			m_skip = 0;
		}
		else
			m_skip = m_size - offset;
		break;
	}
	case std::ios_base::cur:
	{
		if (offset > 0)
			CalcAddDelta(offset);
		else
			m_skip += -offset;
		break;
	}
	case std::ios_base::end:
	{
		if (offset >= 0)
		{
			m_size += offset;
			m_skip = 0;
		}
		else
			m_skip += -offset;
		break;
	}
	default:
		assert(false && "Unsupported seek direction.");
		break;
	}

	// Ensure m_skip is non-negative, this can happen if you call this method
	// with cur or end, and a negative value who's absolute value is greater
	// than the total stream size. If you hit this, you have a bug somewhere.
	assert(m_skip >= 0);

	if (m_skip < 0)
		m_skip = 0;
}
