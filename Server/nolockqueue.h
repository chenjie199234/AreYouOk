#pragma once
#include <memory>
#include <atomic>
template <typename T> class	nolockqueue
{
	public:
		nolockqueue();
		~nolockqueue();
		int pop(std::shared_ptr<T>&);
		bool push(std::shared_ptr<T>&);
		int pop(T&);
		bool push(T&);
	private:
		struct Data
		{
			std::shared_ptr<T> data;
			std::shared_ptr<Data> next;
		};
		std::shared_ptr<Data> head;
		std::shared_ptr<Data> tail;
};
template <typename T>
nolockqueue<T>::nolockqueue()
{
	std::shared_ptr<Data> temphead = std::make_shared<Data>();
	temphead->next = nullptr;
	temphead->data = nullptr;
	head = tail = temphead;
}
template <typename T>
nolockqueue<T>::~nolockqueue()
{
	std::shared_ptr<T> temptask = std::make_shared<T>();
	while(head->next)
	{
		printf("still have data in queue!\n");
		pop(temptask);
	}
}
template <typename T>
int nolockqueue<T>::pop(std::shared_ptr<T>& tempdata)
{
	std::shared_ptr<Data> temphead = std::make_shared<Data>();
	std::shared_ptr<Data> temptail = std::make_shared<Data>();
	std::shared_ptr<Data> oldtail = std::make_shared<Data>();
	if(!temphead|!temptail|!oldtail)
		return -1;//返回-1表明临时指针创建失败
	while (head->next) {
		if (head!=tail) {//表明head在tail前面，正常取出
			do {
				temphead = head;
			} while (!std::atomic_compare_exchange_weak(&head,&temphead,head->next));
			tempdata = head->data;
			return 1;//返回1表明pop成功
		}else {//head追上了tail，但是tail后面还有数据，tail不是最新的tail
			temptail = tail;
			oldtail = temptail;
			while (temptail->next) {
				temptail = temptail->next;
			}
			std::atomic_compare_exchange_weak(&tail,&oldtail,temptail);//不管成功与否，都更新tail，成功则 tail更新为最新，失败表明别的线程更新了tail;
			continue;
		}
	}
	return 0;//返回0表明head->next为空，队列中没有数据。
}
template <typename T>
bool nolockqueue<T>::push(std::shared_ptr<T>& tempdata)
{
	std::shared_ptr<Data> temp = std::make_shared<Data>();
	std::shared_ptr<Data> temptail = std::make_shared<Data>();
	std::shared_ptr<Data> oldtail = std::make_shared<Data>();
	if(!temptail|!oldtail)
		return false;//返回false表明临时指针创建失败
	temp->data = tempdata;
	temp->next = nullptr;
	do {
		temptail = tail;
		oldtail = temptail;
		while (temptail->next) {
			temptail = temptail->next;
		}//获得最新的队尾。
		std::atomic_compare_exchange_weak(&tail,&oldtail,temptail);//不管成功与否，都尝试将tail更新为最新的。成功 表示当前tail最新，失败 表示别的线程更新了tail。
	} while (!std::atomic_compare_exchange_weak(&(tail->next),&(temptail->next),temp));//在最新的tail后插入。
	return true;
}
template <typename T>
int nolockqueue<T>::pop(T& temp)
{
	std::shared_ptr<T> tempdata;
	int returnstate = pop(tempdata);
	if(returnstate>0)
		temp = *tempdata;
	return returnstate;
}
template <typename T>
bool nolockqueue<T>::push(T& temp)
{
	std::shared_ptr<T> tempdata = std::make_shared<T>(temp);
	return push(tempdata);
}
