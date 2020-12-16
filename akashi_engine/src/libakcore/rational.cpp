#include "./rational.h"

#include "./logger.h"

#include <numeric>
#include <stdexcept>

namespace akashi {
    namespace core {

        Rational::Rational(int64_t num, int64_t den) : m_num(num), m_den(den) {
            if (den == 0) {
                throw std::runtime_error("Rational Exception:: den must not be zero");
            }
            this->reduce();
        }

        Rational::Rational(double arg) {
            double num = arg;
            int64_t den = 1;

            while (((double)(int)num) != num) {
                num *= 10;
                den *= 10;

                if (den > INT64_MAX / 10) {
                    AKLOG_WARN(
                        "Rational::Rational(): Failed to convert arg {} to rational number. Using the approximate value instead.",
                        arg);
                    break;
                }
            }

            m_num = (int64_t)num;
            m_den = den;

            this->reduce();
        }

        const int64_t& Rational::num(void) const { return m_num; }

        const int64_t& Rational::den(void) const { return m_den; }

        double Rational::to_decimal(void) const { return (double)m_num / m_den; }

        Rational Rational::operator+(const Rational& other) const {
            auto num = (m_num * other.m_den) + (m_den * other.m_num);
            auto den = m_den * other.m_den;
            return Rational(num, den);
        }

        Rational Rational::operator+=(const Rational& other) {
            m_num = (m_num * other.m_den) + (m_den * other.m_num);
            m_den = m_den * other.m_den;
            this->reduce();
            return *this;
        }

        Rational Rational::operator-(const Rational& other) const {
            auto num = (m_num * other.m_den) - (m_den * other.m_num);
            auto den = m_den * other.m_den;
            return Rational(num, den);
        }

        Rational Rational::operator-=(const Rational& other) {
            m_num = (m_num * other.m_den) - (m_den * other.m_num);
            m_den = m_den * other.m_den;
            this->reduce();
            return *this;
        }

        Rational Rational::operator*(const Rational& other) const {
            auto num = m_num * other.m_num;
            auto den = m_den * other.m_den;
            return Rational(num, den);
        }

        Rational Rational::operator*=(const Rational& other) {
            m_num = m_num * other.m_num;
            m_den = m_den * other.m_den;
            this->reduce();
            return *this;
        }

        Rational Rational::operator/(const Rational& other) const {
            auto num = m_num * other.m_den;
            auto den = m_den * other.m_num;
            return Rational(num, den);
        }

        Rational Rational::operator/=(const Rational& other) {
            m_num = m_num * other.m_den;
            m_den = m_den * other.m_num;
            this->reduce();
            return *this;
        }

        bool Rational::operator<(const Rational& other) const {
            return ((*this) - other).m_num < 0;
        }

        bool Rational::operator<=(const Rational& other) const {
            return ((*this) - other).m_num <= 0;
        }

        bool Rational::operator==(const Rational& other) const {
            return ((*this) - other).m_num == 0;
        }

        bool Rational::operator!=(const Rational& other) const {
            return ((*this) - other).m_num != 0;
        }

        bool Rational::operator>=(const Rational& other) const {
            return ((*this) - other).m_num >= 0;
        }

        bool Rational::operator>(const Rational& other) const {
            return ((*this) - other).m_num > 0;
        }

        void Rational::reduce(void) {
            int64_t gcd = std::gcd(m_num, m_den);
            m_num = m_num / gcd;
            m_den = m_den / gcd;
        }

        Rational to_rational(const Fraction& fraction) {
            Rational rational(fraction.num, fraction.den);
            return rational;
        }

    }
}
