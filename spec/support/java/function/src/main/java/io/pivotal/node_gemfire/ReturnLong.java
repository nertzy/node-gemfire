package io.pivotal.node_gemfire;

import com.gemstone.gemfire.cache.execute.FunctionAdapter;
import com.gemstone.gemfire.cache.execute.FunctionContext;

public class ReturnLong extends FunctionAdapter {

    public void execute(FunctionContext fc) {
        long l = 1;
        fc.getResultSender().lastResult(l);
    }

    public String getId() {
        return getClass().getName();
    }
}
