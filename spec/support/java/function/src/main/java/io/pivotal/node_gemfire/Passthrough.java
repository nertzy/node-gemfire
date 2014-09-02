package io.pivotal.node_gemfire;

import com.gemstone.gemfire.cache.execute.FunctionAdapter;
import com.gemstone.gemfire.cache.execute.FunctionContext;

public class Passthrough extends FunctionAdapter {

    public void execute(FunctionContext fc) {
        fc.getResultSender().lastResult(fc.getArguments());
    }

    public String getId() {
        return getClass().getName();
    }
}
