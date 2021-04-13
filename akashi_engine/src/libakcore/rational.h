#pragma once

#include <cstdint>

namespace akashi {
    namespace core {

        struct Fraction {
            int64_t num;
            int64_t den;
        };

        class Rational final {
          private:
            int64_t m_num = 0;
            int64_t m_den = 1;

          public:
            explicit Rational(int64_t num = 0, int64_t den = 1);
            Rational(double arg);

            const int64_t& num(void) const;
            const int64_t& den(void) const;

            double to_decimal(void) const;

            Fraction to_fraction(void) const { return Fraction{m_num, m_den}; }

            Rational operator+(const Rational& other) const;
            Rational operator+=(const Rational& other);
            Rational operator-(const Rational& other) const;
            Rational operator-=(const Rational& other);
            Rational operator*(const Rational& other) const;
            Rational operator*=(const Rational& other);
            Rational operator/(const Rational& other) const;
            Rational operator/=(const Rational& other);

            bool operator<(const Rational& other) const;
            bool operator<=(const Rational& other) const;
            bool operator==(const Rational& other) const;
            bool operator!=(const Rational& other) const;
            bool operator>=(const Rational& other) const;
            bool operator>(const Rational& other) const;

          private:
            void reduce(void);
        };

        Rational to_rational(const Fraction& fraction);

    }
}
