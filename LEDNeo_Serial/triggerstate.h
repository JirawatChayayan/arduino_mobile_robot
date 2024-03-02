class TriggerInput {
  private:
    byte pin;
    unsigned int statemachine;
    unsigned long t1;
    unsigned int waitMs;
    
  public:
    TriggerInput(byte pin,unsigned int waitMs) 
    {
      this->pin = pin;
      this->waitMs = waitMs;
      init();
    }
    void init() {
      pinMode(pin, INPUT_PULLUP);
      statemachine=0;
    }
    bool getState()
    {
        bool ip = !digitalRead(pin);
        switch(statemachine)
        {
            case 0:
                if(!ip)
                {
                  statemachine = 0;
                }
                else
                {
                  statemachine = 1;
                  t1 = millis();
                }
                return false;
                break;
            case 1:
                if(millis()-t1>waitMs)
                {
                   statemachine = 2;
                   if(!ip)
                   {
                      statemachine = 0;
                      return false;
                   }
                   return true;
                }
                return false;
                break;
            case 2:
                if(!ip)
                {
                  statemachine = 0;
                  return false;
                }
                return true;
                break;
        }
    }
};
