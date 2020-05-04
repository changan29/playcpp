

#ifndef COMMON_SINGLETON_H_
#define COMMON_SINGLETON_H_

template <class T>
class Singleton
{
public:
	static T* GetInst()
	{
		if (_instance == 0)
		{
			_instance = new T();
		}
		return _instance;
	}

private:
	static T* _instance;
};
template <class T>
T* Singleton<T>::_instance = 0;

#endif /* COMMON_SINGLETON_H_ */
