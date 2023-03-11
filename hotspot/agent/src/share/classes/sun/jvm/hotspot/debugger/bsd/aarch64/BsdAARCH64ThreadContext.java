/*
*
*/

package sun.jvm.hotspot.debugger.bsd.aarch64;

import sun.jvm.hotspot.debugger.*;
import sun.jvm.hotspot.debugger.aarch64.*;
import sun.jvm.hotspot.debugger.bsd.*;

public class BsdAARCH64ThreadContext extends AARCH64ThreadContext {
  private BsdDebugger debugger;

  public BsdAARCH64ThreadContext(BsdDebugger debugger) {
    super();
    this.debugger = debugger;
  }

  public void setRegisterAsAddress(int index, Address value) {
    setRegister(index, debugger.getAddressValue(value));
  }

  public Address getRegisterAsAddress(int index) {
    return debugger.newAddress(getRegister(index));
  }
}
