/*
Copyright (c) 2012-2017 MFBS, Fedor Malakhov

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "PoolList.h"
//---------------------------------------------------------------------------
void PoolListInit(PoolListItsTask *ListTasks, unsigned int StartSize)
{
    ListTasks->Count = 0;
    ListTasks->CurrTask = NULL;
    ListTasks->FistTask = NULL;
    CreatePool(&ListTasks->ObjTaskPool, StartSize, sizeof(ObjPoolListTask));
}
//---------------------------------------------------------------------------
ObjPoolListTask* AddPoolStructListObj(PoolListItsTask *ListTasks, void *UserTask)
{
	register ObjPoolListTask *NewPT;
    register ObjPoolListTask *PrevPT;
    register ObjPoolListTask *NextPT;
    register POOL_RECORD_STRUCT *ObjTaskPtr = NULL;

    ObjTaskPtr = GetBuffer(&ListTasks->ObjTaskPool);
	NewPT = (ObjPoolListTask*)ObjTaskPtr->DataPtr;                        
    NewPT->BlkPoolPtr = ObjTaskPtr;
	NewPT->UsedTask = UserTask;
	if (!ListTasks->Count)
	{
		ListTasks->FistTask = NewPT;
	    NewPT->NextTask = NewPT;
	    NewPT->PrevTask = NewPT;
	}
	else
    {
		NextPT = ListTasks->FistTask;
        PrevPT = (ObjPoolListTask*)NextPT->PrevTask;
        NewPT->NextTask = (void*)NextPT;
        NewPT->PrevTask = (void*)PrevPT;
        if (PrevPT) PrevPT->NextTask = (void*)NewPT;
        if (NextPT) NextPT->PrevTask = (void*)NewPT;
      }
	ListTasks->Count++;
	return NewPT;
}
//---------------------------------------------------------------------------
void AddPoolStructList(PoolListItsTask *ListTasks, void *UserTask)
{
	register ObjPoolListTask *NewPT;

	NewPT = AddPoolStructListObj(ListTasks, UserTask);
	return;
}
//---------------------------------------------------------------------------
ObjPoolListTask* AddPoolStructListAboveObj(PoolListItsTask *ListTasks, ObjPoolListTask *AboveTask, void *UserTask)
{
	register ObjPoolListTask *NewPT;
    register ObjPoolListTask *PrevPT;
    register ObjPoolListTask *NextPT;
    register POOL_RECORD_STRUCT *ObjTaskPtr = NULL;

    ObjTaskPtr = GetBuffer(&ListTasks->ObjTaskPool);
	NewPT = (ObjPoolListTask*)ObjTaskPtr->DataPtr;                        
    NewPT->BlkPoolPtr = ObjTaskPtr;
    NewPT->UsedTask = UserTask;
    if (AboveTask == ListTasks->FistTask)
    {
        /* New block should be added before first block in list */
	    NewPT->NextTask = (void*)AboveTask;
	    NewPT->PrevTask = (void*)AboveTask->PrevTask; 
        AboveTask->PrevTask = (void*)NewPT;               
        if (AboveTask->NextTask == (void*)ListTasks->FistTask)
        {
            AboveTask->NextTask = (void*)NewPT;
        }
        else
        {
            NextPT = NewPT->PrevTask;
            if (NextPT) NextPT->NextTask = (void*)NewPT;
        }        
		ListTasks->FistTask = NewPT;
    }
    else
    {
        PrevPT = (ObjPoolListTask*)AboveTask->PrevTask;
        NewPT->NextTask = (void*)AboveTask;
        NewPT->PrevTask = (void*)PrevPT;
        if (PrevPT) PrevPT->NextTask = NewPT;
        AboveTask->PrevTask = NewPT;
    }
    ListTasks->Count++;
    return NewPT;
}
//---------------------------------------------------------------------------
void AddPoolStructListAbove(PoolListItsTask *ListTasks, ObjPoolListTask *AboveTask, void *UserTask)
{
	register ObjPoolListTask *NewPT;
    
    NewPT = AddPoolStructListAboveObj(ListTasks, AboveTask, UserTask);
}
//---------------------------------------------------------------------------
void RemPoolStructList(PoolListItsTask *ListTasks, ObjPoolListTask *PointTask)
{
	register ObjPoolListTask *NextPT;
	register ObjPoolListTask *PrevPT;

	if ( !ListTasks->Count || !PointTask ) return;
	PrevPT = (ObjPoolListTask*)PointTask->PrevTask;
	NextPT = (ObjPoolListTask*)PointTask->NextTask;
    if (PrevPT) PrevPT->NextTask = (void*)NextPT;
    if (NextPT) NextPT->PrevTask = (void*)PrevPT;
    if (PointTask == ListTasks->FistTask) ListTasks->FistTask = NextPT;
    FreeBuffer(&ListTasks->ObjTaskPool, (POOL_RECORD_STRUCT*)PointTask->BlkPoolPtr);
	ListTasks->Count--;
	return;
}
//---------------------------------------------------------------------------
void* GetFistPoolObjectList(PoolListItsTask *ListTasks)
{
	if (!ListTasks) return NULL;
    if (!ListTasks->Count) return NULL;
    ListTasks->CurrTask = (ObjPoolListTask*)ListTasks->FistTask;
    ListTasks->Select = 0;
    return ListTasks->CurrTask;
}
//---------------------------------------------------------------------------
void* GetLastPoolObjectList(PoolListItsTask *ListTasks)
{
	if (!ListTasks) return NULL;
    if (!ListTasks->Count) return NULL;
	if (!ListTasks->FistTask) return NULL;
    ListTasks->CurrTask = (ObjPoolListTask*)(ListTasks->FistTask->PrevTask);
    ListTasks->Select = ListTasks->Count-1;
    return ListTasks->CurrTask;
}
//---------------------------------------------------------------------------
void* GetNextPoolObjectList(PoolListItsTask *ListTasks)
{
	if (!ListTasks) return NULL;
	if (!ListTasks->Count) return NULL;
	if (!ListTasks->CurrTask) return NULL;
	if (!ListTasks->FistTask) return NULL;
    if (ListTasks->FistTask == (ObjPoolListTask*)(ListTasks->CurrTask->NextTask)) return NULL;
    ListTasks->CurrTask = (ObjPoolListTask*)(ListTasks->CurrTask->NextTask);
    ListTasks->Select++;
    return ListTasks->CurrTask;
}
//---------------------------------------------------------------------------
void DestroyPoolListStructs(PoolListItsTask *List)
{
    ObjPoolListTask *PointTask;

    while (List->Count)
    {
        PointTask = List->FistTask;
        RemPoolStructList(List, PointTask);
    }
    DestroyPool(&List->ObjTaskPool);
}
//---------------------------------------------------------------------------
