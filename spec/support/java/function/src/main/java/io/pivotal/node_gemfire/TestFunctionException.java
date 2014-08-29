package io.pivotal.node_gemfire;

import com.gemstone.gemfire.cache.execute.FunctionAdapter;
import com.gemstone.gemfire.cache.execute.FunctionContext;
import com.gemstone.gemfire.cache.execute.FunctionException;
import com.gemstone.gemfire.cache.execute.ResultSender;

public class TestFunctionException extends FunctionAdapter {

    public void execute(FunctionContext fc) {
        throw new FunctionException("Test exception message thrown by server.");
    }

    public String getId() {
        return getClass().getName();
    }
}