typedef void (*Trigger)();

class Button{

    public:
    Button(int input) : input(input){};
    void tick();
    void init(Trigger trigger);

    private:
    Trigger trigger;
    int input;
    unsigned long lMillis;
    bool isTriggered;
};