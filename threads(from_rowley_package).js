// CrossWorks Tasking Library.
//
// Copyright (c) 2004-2014 Rowley Associates Limited.
//
// This file may be distributed under the terms of the License Agreement
// provided with this software.
//
// THIS FILE IS PROVIDED AS IS WITH NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.

function getState(state)
{
  if (state == 0x00)
    return "runnable";
  if (state == 0x1)
    return "timer wait";
  if (state == 0x80)
    return "suspended";
  var stateStr;
  if ((state&~1) == 0x2)
    stateStr = "event wait all";  
  else if ((state&~1) == 0x4)
    stateStr = "event wait all auto clear";  
  else if ((state&~1) == 0x6)
    stateStr = "event wait any";
  else if ((state&~1) == 0x08)     
    stateStr = "event wait any auto clear";     
  else if ((state&~1) ==0x0A)
    stateStr = "semaphore wait";
  else if ((state&~1) ==0x0C)
    stateStr = "message queue post wait";
  else if ((state&~1) ==0x0E)
    stateStr = "message queue receive wait";
  else if ((state&~1) ==0x10)
    stateStr = "mutex wait";
  else
    return "invalid";
  if ((state & 0x01) == 0x01)      
    stateStr += " & timer wait";
  return stateStr;
}

function getregs(x)
{
  var sp = Debug.evaluate("((CTL_TASK_t*)"+x+")->stack_pointer");
  if (sp & 1)
    sp += 63; // FP has been saved
  if (sp & 2)
    sp += 126; // D17-D31 has been saved
  var a = new Array();
  for (i=4;i<12;i++)
    {
      a[i] = TargetInterface.peekWord(sp); 
      sp+=4;
    }
  for (i=0;i<4;i++)
    {
      a[i] = TargetInterface.peekWord(sp);  
      sp+=4;
    }
  a[12] = TargetInterface.peekWord(sp); 
  sp+=4;
  a[14] = TargetInterface.peekWord(sp);  
  sp+=4;
  a[15] = TargetInterface.peekWord(sp); 
  sp+=4;
  a[16] = TargetInterface.peekWord(sp); 
  sp+=4;
  a[13] = sp;
  return a;
}

function gettls(x)
{
  if (x==null)
    x = Debug.evaluate("ctl_task_executing");
  return Debug.evaluate("((CTL_TASK_t*)"+x+")->thread_local_storage");
}

function getname(x)
{
  if (x==null)
    x = Debug.evaluate("ctl_task_executing->name");
  return Debug.evaluate("((CTL_TASK_t*)"+x+")->name");
}

function init()
{
  Threads.setColumns("Name", "Priority", "State", "Time", "Stack Left");
  Threads.setSortByNumber("Time");
}

function update() 
{
  Threads.clear();
  var exe=Debug.evaluate("ctl_task_executing");
  var x=Debug.evaluate("ctl_task_list");

  // If this is running under the simulator or the list is clearly
  // invalid, don't bother populating the threads.
  if (x == 0xcdcdcdcd || (x & 3) != 0)
    return;

  if (x)
    Threads.newqueue("Task List");
  var count=0;
  while (x && count<10)
    {
      var xt = Debug.evaluate("*(CTL_TASK_t*)"+x);
      if (x==exe)
        Threads.add(xt.name, xt.priority, "executing", xt.execution_time, TargetInterface.getRegister("sp")-xt.stack_start, []);
      else
        Threads.add(xt.name, xt.priority, getState(xt.state), xt.execution_time, xt.stack_pointer-xt.stack_start, x);
      x=xt.next;
      count++;
    }
}
