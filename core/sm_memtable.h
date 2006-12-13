#ifndef _INCLUDE_SOURCEMOD_CORE_STRINGTABLE_H_
#define _INCLUDE_SOURCEMOD_CORE_STRINGTABLE_H_

class BaseMemTable
{
public:
	BaseMemTable(unsigned int init_size);
	~BaseMemTable();
public:
	/**
	 * Allocates 'size' bytes of memory.
	 * Optionally outputs the address through 'addr'.
	 * Returns an index >= 0 on success, < 0 on failure.
	 */
	int CreateMem(unsigned int size, void **addr);

	/**
	 * Given an index into the memory table, returns its address.
	 * Returns NULL if invalid.
	 */
	void *GetAddress(int index);

	/**
	 * Scraps the memory table.  For caching purposes, the memory 
	 * is not freed, however subsequent calls to CreateMem() will 
	 * begin at the first index again.
	 */
	void Reset();
private:
	unsigned char *membase;
	unsigned int size;
	unsigned int tail;
};

class BaseStringTable
{
public:
	BaseStringTable(unsigned int init_size);
	~BaseStringTable();
public:
	/** 
	 * Adds a string to the string table and returns its index.
	 */
	int AddString(const char *string);

	/**
	 * Given an index into the string table, returns the associated string.
	 */
	const char *GetString(int str);

	/**
	 * Scraps the string table.  For caching purposes, the memory
	 * is not freed, however subsequent calls to AddString() will 
	 * begin at the first index again.
	 */
	void Reset();

	/**
	 * Returns the parent BaseMemTable that this string table uses.
	 */
	inline BaseMemTable *GetMemTable()
	{
		return &m_table;
	}
private:
	BaseMemTable m_table;
};

#endif //_INCLUDE_SOURCEMOD_CORE_STRINGTABLE_H_