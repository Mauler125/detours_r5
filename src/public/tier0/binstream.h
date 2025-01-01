#pragma once

class CIOStream
{
public:
	enum class Mode_e
	{
		None = 0,
		Read,
		Write,
		ReadWrite, // For existing files only.
		ReadWriteCreate
	};

	CIOStream();
	~CIOStream();

	bool Open(const char* const filePath, const Mode_e mode);
	inline bool Open(const std::string& filePath, const Mode_e mode) { return Open(filePath.c_str(), mode); };

	void Close();
	void Reset();
	void Flush();

	std::streamoff TellGet();
	std::streamoff TellPut();

	void SeekGet(const std::streamoff offset, const std::ios_base::seekdir way = std::ios::beg);
	void SeekPut(const std::streamoff offset, const std::ios_base::seekdir way = std::ios::beg);
	void Seek(const std::streamoff offset, const std::ios_base::seekdir way = std::ios::beg);

	const std::filebuf* GetData() const;
	const std::streamoff GetSize() const;

	bool IsReadMode() const;
	bool IsWriteMode() const;

	bool IsReadable() const;
	bool IsWritable() const;

	bool IsEof() const;

	//-----------------------------------------------------------------------------
	// Purpose: reads any value from the file
	//-----------------------------------------------------------------------------
	template<typename T>
	inline void Read(T& value)
	{
		if (IsReadable())
			m_stream.read(reinterpret_cast<char*>(&value), sizeof(value));
	}

	//-----------------------------------------------------------------------------
	// Purpose: reads any value from the file with specified size
	//-----------------------------------------------------------------------------
	template<typename T>
	inline void Read(T* const value, const size_t size)
	{
		if (IsReadable())
			m_stream.read(reinterpret_cast<char*>(value), size);
	}
	template<typename T>
	inline void Read(T& value, const size_t size)
	{
		if (IsReadable())
			m_stream.read(reinterpret_cast<char*>(&value), size);
	}

	//-----------------------------------------------------------------------------
	// Purpose: reads any value from the file and returns it
	//-----------------------------------------------------------------------------
	template<typename T>
	inline T Read()
	{
		T value{};
		if (!IsReadable())
			return value;

		m_stream.read(reinterpret_cast<char*>(&value), sizeof(value));
		return value;
	}
	bool ReadString(std::string& svOut);
	bool ReadString(char* const pBuf, const size_t nLen);

	//-----------------------------------------------------------------------------
	// Purpose: writes any value to the file
	//-----------------------------------------------------------------------------
	template<typename T>
	inline void Write(const T& value)
	{
		if (!IsWritable())
			return;

		const size_t count = sizeof(value);

		m_stream.write(reinterpret_cast<const char*>(&value), count);
		CalcAddDelta(count);
	}

	//-----------------------------------------------------------------------------
	// Purpose: writes any value to the file with specified size
	//-----------------------------------------------------------------------------
	template<typename T>
	inline void Write(const T* const value, const size_t size)
	{
		if (!IsWritable())
			return;

		m_stream.write(reinterpret_cast<const char*>(value), size);
		CalcAddDelta(size);
	}
	bool WriteString(const std::string& svInput, const bool nullterminate);
	void Pad(const size_t count);

protected:
	void CalcAddDelta(const size_t count);
	void CalcSkipDelta(const std::streamoff offset, const std::ios_base::seekdir way);

private:
	std::fstream            m_stream; // I/O stream.
	std::streamoff          m_size;   // File size.
	std::streamoff          m_skip;   // Amount skipped back.
	std::ios_base::openmode m_flags;  // Stream flags.
	Mode_e                  m_mode;   // Stream mode.
};
