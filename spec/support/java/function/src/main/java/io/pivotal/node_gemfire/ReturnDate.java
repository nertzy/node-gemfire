package io.pivotal.node_gemfire;

import com.gemstone.gemfire.cache.execute.FunctionAdapter;
import com.gemstone.gemfire.cache.execute.FunctionContext;

import java.text.DateFormat;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Date;

public class ReturnDate extends FunctionAdapter {

    public void execute(FunctionContext fc) {
        DateFormat dateFormat = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss.SSS");
        String dateString = "2001-07-04 12:34:56.789";
        try {
            Date date = dateFormat.parse(dateString);
            System.out.println(date);
            fc.getResultSender().lastResult(date);
        } catch (ParseException e) {
            fc.getResultSender().sendException(e);
        }
    }

    public String getId() {
        return getClass().getName();
    }
}
