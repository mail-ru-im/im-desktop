struct JumpListItem
{
	DWORD m_id;
	CAtlString m_title;
	CAtlString m_url;

	JumpListItem(DWORD _id = 0, const CAtlString& _title = L"", const CAtlString& _url = L"")
		: m_id(_id),
		m_title(_title),
		m_url(_url)
	{

	};
	bool IsSeparator()
	{
		return m_id == 0;
	}
};