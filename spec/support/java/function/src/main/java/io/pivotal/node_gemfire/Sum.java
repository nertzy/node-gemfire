package io.pivotal.node_gemfire;

import com.gemstone.gemfire.cache.execute.FunctionAdapter;
import com.gemstone.gemfire.cache.execute.FunctionContext;

import java.util.List;

public class Sum extends FunctionAdapter {

    public void execute(FunctionContext fc) {
        List addendObjects = (List) fc.getArguments();

        Double sum = 0.0;
        for(Object addendObject : addendObjects) {
            sum += (Double) addendObject;
        }

        fc.getResultSender().lastResult(sum);
    }

    public String getId() {
        return getClass().getName();
    }
}
