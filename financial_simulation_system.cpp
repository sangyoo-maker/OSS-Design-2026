#define NOMINMAX
#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <memory>
#include <windows.h>

// 전역 도우미 함수: Java의 DecimalFormat 및 컴마 제거 기능을 대체하기 위함
std::string formatCurrency(double value) {
    std::string res = std::to_string((long long)value);
    int insertPosition = res.length() - 3;
    while (insertPosition > 0) {
        res.insert(insertPosition, ",");
        insertPosition -= 3;
    }
    return res + "원";
}

std::string formatCurrencySpace(double value) {
    std::string res = std::to_string((long long)value);
    int insertPosition = res.length() - 3;
    while (insertPosition > 0) {
        res.insert(insertPosition, ",");
        insertPosition -= 3;
    }
    return res + " 원";
}

std::string formatPercent(double value) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << value << "%";
    return oss.str();
}

std::string replaceAll(std::string str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
    return str;
}

// 입력을 위한 전역 도우미 함수 선언
double promptDouble(const std::string& prompt);
int promptInt(int min, int max, const std::string& prompt);

//사용자의 자산 상태 정보를 캡슐화한 데이터 보유 클래스입니다.
class UserProfile {
private:
    double income;        // 연소득
    double asset;         // 총 자산 (주택담보 기준가 가치)
    double debt;          // 기존 대출 부채
    double interestRate;  // 대출 적용 연금리 (%)
    int loanTerm;         // 사용자 희망 대출 만기 기간 (년 단위)

public:
    UserProfile(double income, double asset, double debt, double interestRate, int loanTerm) {
        this->income = income;
        this->asset = asset;
        this->debt = debt;
        this->interestRate = interestRate;
        this->loanTerm = loanTerm;
    }

    // 다중 가상 시나리오 적용 시 사용
    UserProfile cloneProfile() {
        return UserProfile(this->income, this->asset, this->debt, this->interestRate, this->loanTerm);
    }

    double getIncome() { return income; }
    void setIncome(double income) { this->income = income; }

    double getAsset() { return asset; }
    void setAsset(double asset) { this->asset = asset; }

    double getDebt() { return debt; }
    void setDebt(double debt) { this->debt = debt; }

    double getInterestRate() { return interestRate; }
    void setInterestRate(double interestRate) { this->interestRate = interestRate; }

    int getLoanTerm() { return loanTerm; }
    void setLoanTerm(int loanTerm) { this->loanTerm = loanTerm; }
};

//동적 다형성을 구성하여 하위 시나리오들의 실행 규격을 정의하는 추상 기반 클래스입니다.
class SimulationScenario {
public:
    virtual ~SimulationScenario() {}
    virtual void applyScenario(UserProfile& user) = 0;
    virtual std::string getScenarioName() = 0;
    virtual std::string getScenarioDetails() = 0;
};

// 결혼 시나리오: 결혼으로 인한 지출 발생에 따른 자산 감소 반영
class Marriage : public SimulationScenario {
private:
    double expenseChange;

public:
    Marriage(double expenseChange) {
        this->expenseChange = expenseChange;
    }

    void applyScenario(UserProfile& user) override {
        double updatedAsset = user.getAsset() - this->expenseChange;
        if (updatedAsset < 0) {
            updatedAsset = 0; // 자산 하한 한계 방어
        }
        user.setAsset(updatedAsset);
    }

    std::string getScenarioName() override {
        return "결혼";
    }

    std::string getScenarioDetails() override {
        return "결혼으로 인한 자산 지출: -" + formatCurrency(expenseChange);
    }
};

//첫 취업 시나리오: 취업에 따른 신규 소득 상승폭 반영 처리
class FirstJob : public SimulationScenario {
private:
    double incomeChange;

public:
    FirstJob(double incomeChange) {
        this->incomeChange = incomeChange;
    }

    void applyScenario(UserProfile& user) override {
        user.setIncome(user.getIncome() + this->incomeChange);
    }

    std::string getScenarioName() override {
        return "첫 취업";
    }

    std::string getScenarioDetails() override {
        return "신규 취업 소득 상승: +" + formatCurrency(incomeChange);
    }
};

// 금리 변화 시나리오: 금리 변화 반영
class ChangeRate : public SimulationScenario {
private:
    double interestRateChange;

public:
    ChangeRate(double interestRateChange) {
        this->interestRateChange = interestRateChange;
    }

    void applyScenario(UserProfile& user) override {
        double updatedRate = user.getInterestRate() + this->interestRateChange;
        if (updatedRate < 0.1) {
            updatedRate = 0.1; // 음수 금리 방지 가이드라인 가동
        }
        user.setInterestRate(updatedRate);
    }

    std::string getScenarioName() override {
        return "금리 변화";
    }

    std::string getScenarioDetails() override {
        return "이자율 변동 수치: " + (interestRateChange >= 0 ? std::string("+") : std::string("")) + std::to_string(interestRateChange).substr(0,4) + "%p";
    }
};

//변화 없음 시나리오: 현재 재무 데이터 원형 유지
class NoScenario : public SimulationScenario {
public:
    void applyScenario(UserProfile& user) override {
        // 아무것도 변하지 않음
    }

    std::string getScenarioName() override {
        return "변화 없음";
    }

    std::string getScenarioDetails() override {
        return "변동 조건 미지정";
    }
};

//대출 한도 규제 정책을 저장하고 기준치 준수 적합성을 검증하는 규제 기준 정보 클래스입니다.
class LoanPolicy {
private:
    double maxLTV; // 비율 소수점 (예: 0.70 -> 70%)
    private:
    double maxDSR; // 비율 소수점 (예: 0.40 -> 40%)

public:
    LoanPolicy() {
        this->maxLTV = 0.70;
        this->maxDSR = 0.40;
    }

    LoanPolicy(double maxLTV, double maxDSR) {
        this->maxLTV = maxLTV;
        this->maxDSR = maxDSR;
    }

    bool validatePolicy(double ltv, double dsr) {
        return (ltv / 100.0 <= maxLTV) && (dsr / 100.0 <= maxDSR);
    }

    double getMaxLTV() { return maxLTV; }
    double getMaxDSR() { return maxDSR; }
};

//사용자 프로필의 대출 만기 데이터를 전달받아 금융 연산을 처리하는 계산기 클래스입니다.
class FinanceCalculator {
private:
    LoanPolicy loanPolicy;

    // 연간 원리금 상환 부담액 도출 헬퍼 공식
    double calculateAnnualAmortization(double debt, double annualRate, int years) {
        if (debt <= 0) return 0.0;
        double r = (annualRate / 100.0) / 12.0;
        int n = years * 12;
        if (r == 0) return (debt / n) * 12;
        double monthlyPayment = (debt * r * std::pow(1 + r, n)) / (std::pow(1 + r, n) - 1);
        return monthlyPayment * 12;
    }

public:
    FinanceCalculator() {
        this->loanPolicy = LoanPolicy();
    }

    // LTV 계산
    double calculateLTV(UserProfile& user) {
        if (user.getAsset() <= 0) return 0.0;
        return (user.getDebt() / user.getAsset()) * 100.0;
    }

    // DSR 계산 (기존 부채의 상환 부담 대조를 위해 10년 원리금 균등분할 상정)
    double calculateDSR(UserProfile& user) {
        if (user.getIncome() <= 0) return 100.0;
        double annualRepayment = calculateAnnualAmortization(user.getDebt(), user.getInterestRate(), 10);
        return (annualRepayment / user.getIncome()) * 100.0;
    }

    // 기존 부채에 대해 실제로 매달 납부 중인 예상 상환액 계산 (10년 원리금 균등상환 상정)
    public:
    double calculateExistingMonthlyPayment(UserProfile& user) {
        if (user.getDebt() <= 0) return 0.0;
        return calculateAnnualAmortization(user.getDebt(), user.getInterestRate(), 10) / 12.0;
    }

    // 주택담보한도(LTV 70%)와 소득대비 상환한도(DSR 40%) 규제를 동시에 충족하는
    double calculateLoanLimit(UserProfile& user) {
        // 1. LTV 기준 잔여 대출 한도액 한계점 도출
        double ltvMaxAllowedDebt = user.getAsset() * loanPolicy.getMaxLTV();
        double ltvLimit = ltvMaxAllowedDebt - user.getDebt();
        if (ltvLimit < 0) ltvLimit = 0;

        // 2. DSR 기준 잔여 허용 가능 한도액 역산
        double dsrMaxAllowedAnnualRepayment = user.getIncome() * loanPolicy.getMaxDSR();
        double currentAnnualRepayment = calculateAnnualAmortization(user.getDebt(), user.getInterestRate(), 10);
        double availableAnnualRepayment = dsrMaxAllowedAnnualRepayment - currentAnnualRepayment;

        if (availableAnnualRepayment <= 0) {
            return 0.0; // 추가 대출 여력 없음
        }

        // 3. 사용자가 지정한 만기 연수(user.getLoanTerm())를 기준으로 개월 수 환산 및 한도 역산
        double monthlyRate = (user.getInterestRate() / 100.0) / 12.0;
        int totalMonths = user.getLoanTerm() * 12; // 사용자 정의 변수 대입
        double availableMonthlyRepayment = availableAnnualRepayment / 12.0;

        double dsrLimit;
        if (monthlyRate == 0) {
            dsrLimit = availableMonthlyRepayment * totalMonths;
        } else {
            // 원리금 균등 분할 상환식 역산: Principal = MonthlyPayment / [ r(1+r)^n / ((1+r)^n - 1) ]
            double factor = (monthlyRate * std::pow(1 + monthlyRate, totalMonths)) / (std::pow(1 + monthlyRate, totalMonths) - 1);
            dsrLimit = availableMonthlyRepayment / factor;
        }

        // LTV 규제 한계치와 DSR 규제 한계치 중 보다 보수적인 최소값을 최종 확정 한도
        double finalLimit = std::min(ltvLimit, dsrLimit);
        return std::floor(finalLimit / 10000.0) * 10000.0; // 만원 단위 이하 절사 처리
    }

    // 신규 추가 대출 최대 승인액에 비례한 예상 월 원리금 상환 부담금액 (사용자가 지정한 만기 적용)
    double calculateMonthlyPayment(UserProfile& user) {
        double loanLimit = calculateLoanLimit(user);
        if (loanLimit <= 0) return 0.0;

        double monthlyRate = (user.getInterestRate() / 100.0) / 12.0;
        int totalMonths = user.getLoanTerm() * 12; // 사용자 정의 변수 대입

        if (monthlyRate == 0) {
            return loanLimit / totalMonths;
        }
        return (loanLimit * monthlyRate * std::pow(1 + monthlyRate, totalMonths)) / (std::pow(1 + monthlyRate, totalMonths) - 1);
    }
};

// 연산된 정량 지표(LTV, DSR)에 비추어 리스크 위험 등급을 분석하고 피드백을 수립합니다.
class RiskAnalyzer {
public:
    std::string analyzeRisk(double ltv, double dsr) {
        if (ltv > 70.0 || dsr > 40.0) {
            return "매우 위험";
        } else if (ltv > 50.0 || dsr > 30.0) {
            return "위험";
        } else if (ltv > 30.0 || dsr > 15.0) {
            return "보통";
        } else {
            return "안전";
        }
    }

    std::string generateFeedback(std::string riskLevel) {
        if (riskLevel == "매우 위험") {
            return "금융당국의 대출 제한 가이드라인을 초과했거나 임계선에 걸쳐 있습니다. 자금 계획은 채무 감축을 최우선으로 진행해야 합니다.";
        } else if (riskLevel == "위험") {
            return "월 소득 대비 부채 비중이 높은 편입니다. 변동 비용 및 생활비 지출 규모를 축소하고 추가 대출 유치는 가급적 자제하는 것이 바람직합니다.";
        } else if (riskLevel == "보통") {
            return "무난한 재무 건전성을 확보하고 있습니다. 금리 변동 위험을 완화하기 위해 평소 정기적인 저축 비중과 현금 소유를 하시기 바랍니다.";
        } else if (riskLevel == "안전") {
            return "재무 상태가 대단히 튼튼하고 여력이 충분합니다. 대출 유치 부담이 거의 없는 안정 지대로서 적극적인 장기 연금 가입이나 포트폴리오 다각화 투자를 모색해 볼 만합니다.";
        } else {
            return "재무 데이터를 면밀히 관리해 주십시오.";
        }
    }
};

//사용자 이해를 돕기 위한 필수 용어 정리 및 사전 가이드를 관리합니다.
class FinanceGlossary {
public:
    std::string getTermMeaning(std::string term) {
        std::transform(term.begin(), term.end(), term.begin(), ::toupper);
        if (term == "LTV") {
            return "LTV (주택담보대출비율, Loan to Value Ratio): 자산 담보 가치 대비 최대로 대출을 받을 수 있는 비율을 뜻합니다.";
        } else if (term == "DSR") {
            return "DSR (총부채원리금상환비율, Debt Service Ratio): 가계 총소득 중 매년 상환해야 하는 원금과 이자의 비율을 의미합니다.";
        } else {
            return "올바른 금융 용어에 대한 정리가 성공적인 재무 설계의 첫걸음입니다.";
        }
    }

    std::string showGuide() {
        return "\n LTV (주택담보대출비율): 부동산 가치 대비 빚의 크기를 조절하는 안전 규제 척도입니다.\n DSR (총부채원리금상환비율): 실질 상환 능력을 엄격히 통제하는 주된 연소득 소모 비율 지표입니다.\n=========================================================================";
    }
};

// 특정 시나리오 연산이 완료된 최종 보고 지표들을 담고 출력해 주는 결과 데이터 보관 클래스입니다.
class SimulationResult {
private:
    std::string scenarioName;
    std::string scenarioDetails;
    double loanLimit;
    double monthlyPayment;
    double existingMonthlyPayment; // 기존 부채 월 상환 부담액 필드 추가
    double ltvValue;
    double dsrValue;
    std::string riskLevel;
    std::string feedback;
    int loanTerm; // 사용자가 선택한 만기 연수 추적용

public:
    SimulationResult(std::string scenarioName, std::string scenarioDetails, double loanLimit, double monthlyPayment, 
                     double existingMonthlyPayment, double ltvValue, double dsrValue, std::string riskLevel, std::string feedback, int loanTerm) {
        this->scenarioName = scenarioName;
        this->scenarioDetails = scenarioDetails;
        this->loanLimit = loanLimit;
        this->monthlyPayment = monthlyPayment;
        this->existingMonthlyPayment = existingMonthlyPayment;
        this->ltvValue = ltvValue;
        this->dsrValue = dsrValue;
        this->riskLevel = riskLevel;
        this->feedback = feedback;
        this->loanTerm = loanTerm;
    }

    void displayResult() {
        std::cout << "\n===== 시뮬레이션 결과 =====" << std::endl;
        std::cout << " LTV : " << formatPercent(this->ltvValue) << std::endl;
        std::cout << " DSR : " << formatPercent(this->dsrValue) << std::endl;
        std::cout << std::endl;
        std::cout << " [현재 기준] 기존 부채 예상 월 상환액 (10년 원리금 분할 가정):" << std::endl;
        std::cout << " " << formatCurrencySpace(this->existingMonthlyPayment) << std::endl;
        std::cout << std::endl;
        std::cout << " [미래 가상] 신규 최대 추가 대출 가능 금액 :" << std::endl;
        std::cout << " " << formatCurrencySpace(this->loanLimit) << std::endl;
        std::cout << std::endl;
        std::cout << " [미래 가상] 신규 대출 최대 신청 시 예상 월 상환액 (" << this->loanTerm << "년 원리금균등 상정):" << std::endl;
        std::cout << " " << formatCurrencySpace(this->monthlyPayment) << std::endl;
        std::cout << std::endl;
        std::cout << " 위험도 : " << this->riskLevel << std::endl;
        std::cout << " 맞춤 컨설팅 피드백: " << this->feedback << std::endl;
        std::cout << "==========================\n" << std::endl;
    }

    // 비교 및 저장을 위한 Getter 메소드들
    std::string getScenarioName() { return scenarioName; }
    std::string getScenarioDetails() { return scenarioDetails; }
    double getLoanLimit() { return loanLimit; }
    double getMonthlyPayment() { return monthlyPayment; }
    double getExistingMonthlyPayment() { return existingMonthlyPayment; }
    double getLtvValue() { return ltvValue; }
    double getDsrValue() { return dsrValue; }
    std::string getRiskLevel() { return riskLevel; }
    std::string getFeedback() { return feedback; }
    int getLoanTerm() { return loanTerm; }
};

//여러 상황의 지표들을 대조 분석하고 정량적으로 최적안을 제안하는 비교 엔진입니다.
class ComparisonEngine {
public:
    void compareResults(std::vector<SimulationResult>& results) {
        if (results.empty()) return;

        std::cout << "\n===== 결과 비교 =====" << std::endl;

        SimulationResult* bestChoice = nullptr;
        double maxLimit = -1.0;

        for (size_t i = 0; i < results.size(); ++i) {
            SimulationResult& res = results[i];
            std::cout << "\n[" << res.getScenarioName() << "]" << std::endl;
            std::cout << " LTV : " << formatPercent(res.getLtvValue()) << std::endl;
            std::cout << " DSR : " << formatPercent(res.getDsrValue()) << std::endl;
            std::cout << " 기존 부채 월 상환액 : " << formatCurrencySpace(res.getExistingMonthlyPayment()) << " (10년 상정)" << std::endl;
            std::cout << " 신규 대출 가능 금액 : " << formatCurrencySpace(res.getLoanLimit()) << " (" << std::to_string(res.getLoanTerm()) << "년 상환 기준)" << std::endl;
            std::cout << " 신규 대출 월 상환액 : " << formatCurrencySpace(res.getMonthlyPayment()) << std::endl;
            std::cout << " 위험도 등급         : " << res.getRiskLevel() << std::endl;

            // 한도 확보성 관점에서 가장 효율성이 우수한 시나리오 탐색 및 지정
            if (res.getLoanLimit() > maxLimit) {
                maxLimit = res.getLoanLimit();
                bestChoice = &res;
            }
        }

        std::cout << "\n--------------------------------------------------" << std::endl;
        if (bestChoice != nullptr) {
            std::cout << " ☞ 진단 판정: '" << bestChoice->getScenarioName() << "' 시나리오가 대출 조달 및 자금 측면에서 가장 유리합니다." << std::endl;
        }
        std::cout << "=====================" << std::endl;
    }
};

//result.txt 파일로의 누적 저장과 가상의 원격 보고서 제어 클래스입니다.
class ResultManager {
private:
    const std::string SAVE_FILE = "result.txt";

public:
    void saveResult(SimulationResult& result) {
        std::ofstream outFile(SAVE_FILE, std::ios::app);

        if (outFile.is_open()) {
            std::time_t now = std::time(nullptr);
            std::string timeStr = std::ctime(&now);
            if (!timeStr.empty() && timeStr.back() == '\n') timeStr.pop_back();

            outFile << "====== FINANCIAL FUTURES SIMULATION REPORT ======" << std::endl;
            outFile << "생성 시점: " << timeStr << std::endl;
            outFile << "가정 시나리오 : " << result.getScenarioName() << " (" + result.getScenarioDetails() + ")" << std::endl;
            outFile << "LTV 분석 결과 : " << formatPercent(result.getLtvValue()) << std::endl;
            outFile << "DSR 분석 결과 : " << formatPercent(result.getDsrValue()) << std::endl;
            outFile << "기존 부채 예상 월상환액: " << formatCurrency(result.getExistingMonthlyPayment()) << " (10년 원리금 상정)" << std::endl;
            outFile << "최대 추가 대출한도     : " << formatCurrency(result.getLoanLimit()) << std::endl;
            outFile << "신규 대출 월별상환액   : " << formatCurrency(result.getMonthlyPayment()) << " (" << std::to_string(result.getLoanTerm()) << "년 원리금 균등상환)" << std::endl;
            outFile << "위험 통제 등급 : " << result.getRiskLevel() << std::endl;
            outFile << "컨설팅 피드백 : " << result.getFeedback() << std::endl;
            outFile << "--------------------------------------------------\n" << std::endl;

            std::cout << "\n[저장 성공] 시뮬레이션 지표 보고서가 로컬 파일 '" << SAVE_FILE << "' 저장소에 누적 저장 완료되었습니다." << std::endl;
            outFile.close();
        } else {
            std::cout << "\n[오류] 디스크 출력 입출력 실패로 보고서 영구 기록을 생략합니다: " << "파일을 열 수 없습니다." << std::endl;
        }
    }
};

//사용자 프로필의 데이터 변동을 다형성 기법으로 적용 및 계산하여 결과를 반환합니다.
class SimulationService {
private:
    std::shared_ptr<UserProfile> userProfile;
    FinanceCalculator calculator;
    RiskAnalyzer analyzer;
    std::shared_ptr<SimulationScenario> scenario;

public:
    SimulationService() {
        this->calculator = FinanceCalculator();
        this->analyzer = RiskAnalyzer();
        this->scenario = std::make_shared<NoScenario>(); // 디폴트 '변화 없음' 시나리오 장착
    }

    void setUserProfile(UserProfile profile) {
        this->userProfile = std::make_shared<UserProfile>(profile);
    }

    void setScenario(std::shared_ptr<SimulationScenario> scenario) {
        this->scenario = scenario;
    }

    void applyScenario() {
        if (this->scenario != nullptr && this->userProfile != nullptr) {
            this->scenario->applyScenario(*(this->userProfile));
        }
    }

    SimulationResult runSimulation() {
        // Deep Copy 기법을 사용하여 오리지널 사용자 프로필 불변성 보호
        UserProfile clonedProfile = this->userProfile->cloneProfile();

        // 1. 다형적으로 바인딩된 시나리오의 변수 적용 (applyScenario 호출)
        this->scenario->applyScenario(clonedProfile);

        // 2. 금융 수학 공식 연산 가동
        double ltv = calculator.calculateLTV(clonedProfile);
        double dsr = calculator.calculateDSR(clonedProfile);
        double limit = calculator.calculateLoanLimit(clonedProfile);
        double monthly = calculator.calculateMonthlyPayment(clonedProfile);
        double existingMonthly = calculator.calculateExistingMonthlyPayment(clonedProfile);

        // 3. 재무 지표 기반 가이드라인 처방 분석
        std::string risk = analyzer.analyzeRisk(ltv, dsr);
        std::string fb = analyzer.generateFeedback(risk);

        // 4. 구조화된 최종 결과 리포트 패키지 합성 조립 반환
        return SimulationResult(
                this->scenario->getScenarioName(),
                this->scenario->getScenarioDetails(),
                limit,
                monthly,
                existingMonthly, // 기존 부채 월 상환 부담액 연동
                ltv,
                dsr,
                risk,
                fb,
                clonedProfile.getLoanTerm()
        );
    }

    SimulationResult generateResult() {
        return runSimulation();
    }
};

/**
[금융 미래 시뮬레이션 시스템 - Financial Future Simulation System]
본 프로그램은 사용자의 재무 정보(소득, 자산, 부채, 이자율, 대출만기)를 기반으로
미래 시나리오(결혼, 첫 취업, 금리 변화)를 다형적으로 적용하여 LTV, DSR, 대출 가능액 및 월 상환금을 계산합니다.
 */
int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    SimulationService service;
    std::vector<SimulationResult> resultHistory;

    std::cout << "=======================================================================" << std::endl;
    std::cout << "   금융 미래 시뮬레이션 시스템 (Financial Future Simulation System) 가동" << std::endl;
    std::cout << "=======================================================================" << std::endl;

    // 금융 가이드 안내문 및 사전 용어 설명 출력
    FinanceGlossary glossary;
    std::cout << glossary.showGuide() << std::endl;

    // 1단계: 사용자 기본 재무 정보 입력
    std::cout << "\n[1단계] 사용자의 현재 재무 상태 정보를 입력하십시오." << std::endl;
    double income = promptDouble(" ▶ 연소득 입력 (단위: 원, 예: 45000000): ");
    double asset = promptDouble(" ▶ 주택 담보 가치 입력(구매하고 싶은 아파트의 가격) (단위: 원, 예: 250000000): ");
    double debt = promptDouble(" ▶ 기존 부채 총액 입력(아파트 구매한 대출할 금액 또는 기존의 부채) (단위: 원, 예: 50000000): ");
    double interestRate = promptDouble(" ▶ 희망/기준 연금리 입력 (단위: %, 예: 4.2): ");

    // 신규 대출 만기는 20년 고정으로 대입 처리
    UserProfile userProfile(income, asset, debt, interestRate, 20);

    bool continueLoop = true;
    while (continueLoop) {
        // 2단계: 미래 가상 시나리오 선택
        std::cout << "\n[2단계] 가상 적용할 미래 시나리오를 선택하십시오." << std::endl;
        std::cout << " 1. 결혼 (목돈 지출로 인한 자산 감소)" << std::endl;
        std::cout << " 2. 첫 취업 (새로운 소득원으로 인한 소득 증가)" << std::endl;
        std::cout << " 3. 금리 변화 (거시 경제 변동으로 인한 연금리 증감)" << std::endl;
        std::cout << " 4. 변화 없음 (현재 상황 고수)" << std::endl;
        std::cout << " ☞ 선택 (1~4): ";
        int scenarioChoice = promptInt(1, 4, "");

        std::shared_ptr<SimulationScenario> scenario = nullptr;

        // 3단계: 시나리오 유형별 추가 변수 설정 및 다형적 객체 매핑
        switch (scenarioChoice) {
            case 1: {
                double expenseChange = promptDouble("\n ▶ 결혼 준비로 인한 지출금(자산 차감액) 입력 (원): ");
                scenario = std::make_shared<Marriage>(expenseChange);
                break;
            }
            case 2: {
                double incomeChange = promptDouble("\n ▶ 첫 취업으로 인한 연소득 증가폭 입력 (원): ");
                scenario = std::make_shared<FirstJob>(incomeChange);
                break;
            }
            case 3: {
                double interestRateChange = promptDouble("\n ▶ 금리 변동치 입력 (%, 인상은 양수, 인하는 음수 입력): ");
                scenario = std::make_shared<ChangeRate>(interestRateChange);
                break;
            }
            case 4:
                scenario = std::make_shared<NoScenario>();
                break;
        }

        // 4단계: SimulationService 설정 및 가동
        service.setUserProfile(userProfile);
        service.setScenario(scenario);
        
        std::cout << "\n>> 금융 시뮬레이션 알고리즘 연산 가동 중..." << std::endl;
        SimulationResult result = service.runSimulation();
        resultHistory.push_back(result);

        // 5단계: 정산 결과 출력
        result.displayResult();

        // 6단계: 대조 비교를 위한 추가 시나리오 여부 결정
        std::cout << "추가 시나리오를 설계하여 대조 비교하시겠습니까?" << std::endl;
        std::cout << " 1. 예 (다른 변수의 시나리오 설계 단계로 이동)" << std::endl;
        std::cout << " 2. 아니오 (설계 종료 및 최종 정산 단계로 이동)" << std::endl;
        int addChoice = promptInt(1, 2, " ☞ 선택 (1~2): ");
        if (addChoice == 2) {
            continueLoop = false;
        }
    }

    // 7단계: 결과가 2개 이상일 때 비교 분석 엔진 구동
    if (resultHistory.size() >= 2) {
        ComparisonEngine comparisonEngine;
        comparisonEngine.compareResults(resultHistory);
    }

    // 8단계: 파일 영구 저장 여부 물은 뒤 물리 저장소 기록 실행
    std::cout << "\n결과를 저장하시겠습니까?" << std::endl;
    std::cout << " 1. 예 (저장)" << std::endl;
    std::cout << " 2. 아니오 (프로그램 종료)" << std::endl;
    int saveChoice = promptInt(1, 2, " ☞ 선택 (1~2): ");

    if (saveChoice == 1 && !resultHistory.empty()) {
        ResultManager resultManager;
        // 직전에 돌려본 가장 최신의 시뮬레이션 결과 리포트를 영구 디스크에 출력
        SimulationResult& latestResult = resultHistory.back();
        resultManager.saveResult(latestResult);
    }

    std::cout << "\n=======================================================================" << std::endl;
    std::cout << "  금융 시뮬레이터가 안전하게 종료되었습니다. 유연한 자산 설계를 응원합니다." << std::endl;
    std::cout << "=======================================================================" << std::endl;
    
    return 0;
}

//데이터 신뢰성을 확보하기 위한 입력 유효성 검증 
double promptDouble(const std::string& prompt) {
    std::string input;
    while (true) {
        std::cout << prompt;
        std::getline(std::cin, input);
        
        // trim 빈칸 제거
        input.erase(input.begin(), std::find_if(input.begin(), input.end(), [](unsigned char ch) {
            return !std::isspace(ch);
        }));
        input.erase(std::find_if(input.rbegin(), input.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
        }).base(), input.end());

        // 컴마(,) 제거
        input = replaceAll(input, ",", "");

        try {
            size_t idx;
            double value = std::stod(input, &idx);
            if (idx != input.length()) throw std::invalid_argument("인자 불일치");

            if (value < 0 && prompt.find("변동치") == std::string::npos) {
                std::cout << " [경고] 음수 값은 입력할 수 없습니다. 다시 시도해 주십시오." << std::endl;
                continue;
            }
            return value;
        } catch (...) {
            std::cout << " [오류] 올바른 숫자 형식(실수/정수)으로 입력해 주십시오." << std::endl;
        }
    }
}

int promptInt(int min, int max, const std::string& prompt) {
    std::string input;
    while (true) {
        if (!prompt.empty()) {
            std::cout << prompt;
        }
        std::getline(std::cin, input);
        
        // trim 빈칸 제거
        input.erase(input.begin(), std::find_if(input.begin(), input.end(), [](unsigned char ch) {
            return !std::isspace(ch);
        }));
        input.erase(std::find_if(input.rbegin(), input.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
        }).base(), input.end());

        try {
            size_t idx;
            int value = std::stoi(input, &idx);
            if (idx != input.length()) throw std::invalid_argument("인자 불일치");

            if (value >= min && value <= max) {
                return value;
            }
            std::printf(" [경고] %d에서 %d 사이의 유효 번호/값만 입력해 주십시오.\n", min, max);
        } catch (...) {
            std::cout << " [오류] 정수 번호 양식으로 입력해 주십시오." << std::endl;
        }
    }
}
