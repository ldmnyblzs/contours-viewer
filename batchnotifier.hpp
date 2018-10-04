#ifndef BATCHNOTIFIER_HPP
#define BATCHNOTIFIER_HPP

class BatchNotifier {
public:
    virtual void file_started() const {
    }
    virtual void file_finished() const {
    }
    virtual ~BatchNotifier() {
    }
};

#endif // BATCHNOTIFIER_HPP
