package io.pivotal.node_gemfire;

import com.gemstone.gemfire.cache.execute.FunctionAdapter;
import com.gemstone.gemfire.cache.execute.FunctionContext;
import com.gemstone.gemfire.cache.execute.FunctionException;

public class TestFunctionExceptionResult extends FunctionAdapter {

    public void execute(FunctionContext fc) {
        fc.getResultSender().sendResult("First result");
        fc.getResultSender().sendException(new Exception("Test exception message sent by server."));
    }

    public String getId() {
        return getClass().getName();
    }
}