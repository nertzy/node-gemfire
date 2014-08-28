package io.pivotal.node_gemfire;

import com.gemstone.gemfire.cache.execute.FunctionAdapter;
import com.gemstone.gemfire.cache.execute.FunctionContext;

public class TestFunction extends FunctionAdapter {

    public void execute(FunctionContext fc) {
        fc.getResultSender().lastResult("TestFunction succeeded.");
    }

    public String getId() {
        return getClass().getName();
    }
}
