package io.pivotal.node_gemfire;

import com.gemstone.gemfire.cache.execute.FunctionAdapter;
import com.gemstone.gemfire.cache.execute.FunctionContext;

import java.util.ArrayList;
import java.util.List;

public class ReturnList extends FunctionAdapter {

    public void execute(FunctionContext fc) {
        List<Object> list = new ArrayList<Object>();
        list.add("foo");
        list.add("bar");
        list.add(1);
        list.add(2.2d);

        List<Object> subList = new ArrayList<Object>();
        subList.add(3);
        subList.add(4.4d);
        list.add(subList);

        fc.getResultSender().lastResult(list);
    }

    public String getId() {
        return getClass().getName();
    }
}
