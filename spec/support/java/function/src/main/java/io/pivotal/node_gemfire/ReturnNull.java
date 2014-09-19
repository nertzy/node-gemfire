package io.pivotal.node_gemfire;

import com.gemstone.gemfire.cache.execute.FunctionAdapter;
import com.gemstone.gemfire.cache.execute.FunctionContext;

public class ReturnNull extends FunctionAdapter {

    public void execute(FunctionContext fc) {
        fc.getResultSender().lastResult(null);
    }

    public String getId() {
        return getClass().getName();
    }
}
