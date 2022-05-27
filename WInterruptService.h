#ifndef __INTERRUPT_SERVICE_
#define __INTERRUPT_SERVICE_
class InterruptService
{
	public:
	virtual void service(void)=0;
};
#endif