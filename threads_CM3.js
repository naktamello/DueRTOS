function decode_stack(sp)
{
  WScript.Echo("stack "+sp.toString(16)+"\n");
  var i;
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
  if (a[16] & (1<<9)) // Stack has been 8-byte aligned
    sp += 4;
  a[13] = sp;

  return a;
}

function add_task(task, state)
{
  var tcb, task_name, current_task, regs;

  current_task = Debug.evaluate("pxCurrentTCB");
  tcb = Debug.evaluate("*(tskTCB *)" + task);

  task_name = Debug.evaluate("(char*)&(*(tskTCB *)" + task + ").pcTaskName[0]");
 // task_name = "#" + tcb.uxTCBNumber + " \"" + task_name + "\""; //this is undefined in default settings

//return if current_task, it is catched in update()
  if (task == current_task)
  {
    //state = "executing";
    //regs = [];
    return;
  }

  else
  {
    regs = decode_stack(tcb.pxTopOfStack);
  }
  
  Threads.add(task_name, tcb.uxPriority, state, regs);
}

function add_list(list, state)
{
  var i, index, item, task;

  if (list && list.uxNumberOfItems>0)
  {
    index = list.xListEnd.pxNext;

    for (i = 0; i < list.uxNumberOfItems; i++)
    {
      item = Debug.evaluate("*(ListItem_t *)" + index);
      task = item ? item.pvOwner : 0;

      if (task)
        add_task(task, state);

      index = item.pxNext;

      if (i > 10)
        break;
    }
  }
}


function update() 
{
  var i, list, lists, max_priority, current_tcb, current_task_name, current_task_priority, regs;
  regs = [];
  
  Threads.clear();
 

  current_tcb = Debug.evaluate("pxCurrentTCB");

    if( (current_tcb == 0) || (current_tcb == NULL) )
    return;
    
    else
    {
      current_task_name = Debug.evaluate("(char*)&(*(tskTCB *)" + current_tcb + ").pcTaskName[0]");
      //current_task_priority = Debug.evaluate("(char*)&(*(tskTCB *)" + current_tcb + ").uxPriority");
      Threads.newqueue("Current");
      current_tcb = Debug.evaluate("*(tskTCB *)" + current_tcb);
      Threads.add(current_task_name, current_tcb.uxPriority, "executing", regs);
    }


  max_priority = Debug.evaluate("uxTopReadyPriority");  

  /* Ready tasks */
  Threads.newqueue("Ready");
  for (i = max_priority; i >= 0; i--)
  {
     list = Debug.evaluate("pxReadyTasksLists[" + i + "]");
     add_list(list, "runnable");
  }

  /* Blocked (delayed) tasks */
  Threads.newqueue("Blocked");
  //list = Debug.evaluate("xDelayedTaskList1");
  list = Debug.evaluate("*pxDelayedTaskList");
  if (list)
  {
    //list = Debug.evaluate("*(xList *)" + list);
    add_list(list, "timer wait");
  }
  //list = Debug.evaluate("xDelayedTaskList2");
  list = Debug.evaluate("*pxOverflowDelayedTaskList");
  if (list)
  {
    //list = Debug.evaluate("*(xList *)" + list);
    add_list(list, "timer wait");
  }

  /* Suspended tasks */
  Threads.newqueue("Suspended");
  list = Debug.evaluate("xSuspendedTaskList");
  if (list)
  {
    add_list(list, "suspended");
  } 
}

