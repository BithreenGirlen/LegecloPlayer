#ifndef WINDOW_MENU_H_
#define WINDOW_MENU_H_

#include <Windows.h>

namespace window_menu
{
	struct MenuItem
	{
		UINT_PTR id = 0;
		const wchar_t* name = nullptr;
		HMENU child = nullptr;
	};

	class MenuBuilder
	{
	public:
		template <size_t itemCount>
		MenuBuilder(const MenuItem(&menuItems)[itemCount])
		{
			m_hMenu = ::CreateMenu();

			for (const auto& menuItem : menuItems)
			{
				if (IsValid())
				{
					if (menuItem.child == nullptr)
					{
						m_lastResult = menuItem.name == nullptr ?
							::AppendMenuW(m_hMenu, MF_SEPARATOR, 0, nullptr) :
							::AppendMenuW(m_hMenu, MF_STRING, menuItem.id, menuItem.name);
					}
					else
					{
						m_lastResult = ::AppendMenuW(m_hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(menuItem.child), menuItem.name);
					}
				}
				else
				{
					Destroy();
					break;
				}
			}
		}
		~MenuBuilder()
		{
			Destroy();
		}

		HMENU Get() const { return m_hMenu; }
	private:
		struct Result
		{
			enum { Error = 0, Success };
		};

		HMENU m_hMenu = nullptr;
		BOOL m_lastResult = Result::Success;

		bool IsValid() const { return (m_hMenu != nullptr) && (m_lastResult != Result::Error); }

		void Destroy()
		{
			if (m_hMenu != nullptr && m_lastResult == Result::Error)
			{
				::DestroyMenu(m_hMenu);
				m_hMenu = nullptr;
			}
		}
	};

	HMENU GetMenuInBar(HWND hOwnerWindow, unsigned int index);
	bool SetMenuCheckState(HMENU hMenu, unsigned int index, bool checked);
	void EnableMenuItems(HMENU hMenu, const unsigned int* itemIndices, size_t indexCount, bool toEnable);
	template<size_t indexCount>
	void EnableMenuItems(HMENU hMenu, const unsigned int(&itemIndices)[indexCount], bool toEnable)
	{
		EnableMenuItems(hMenu, itemIndices, indexCount, toEnable);
	}
} /* namespace window_menu */

#endif // !WINDOW_MENU_H_

