function [expectedBlerVector] = getLenaHarqBler( ...
        iterLen, mcsIndex, snrRcvd_avg, hvar, M_Harq, ...
        bgType, cbsTarget, Nrb)
%GETLENAHARQBLER Auto-generated from UlRxPacketTrace (par=3, srs=80 fixed)
% expectedBlerVector = [p0, p1, p2, p3] where p_k is mean TBLER for rv=k
% NOTE: SINR is taken from folder name sinr*_par3_srs80 (file SINR(dB) ignored).

%#ok<*INUSD>
sinrDb = round(snrRcvd_avg); % assume caller passes ~folder SINR(dB)
expectedBlerVector = [NaN, NaN, NaN, NaN];

switch sinrDb
    case 10
        switch mcsIndex
            case 0
                expectedBlerVector = [0.000129, 0.805690, 0.865164, 0.473989];
            case 1
                expectedBlerVector = [0.000292, 0.512591, 0.893291, 0.515929];
            case 2
                expectedBlerVector = [0.000657, 0.336892, 0.897005, 0.523517];
            case 3
                expectedBlerVector = [0.001541, 0.269473, 0.738805, 0.495690];
            case 4
                expectedBlerVector = [0.002802, 0.255357, 0.538072, 0.511996];
            case 5
                expectedBlerVector = [0.004669, 0.183274, 0.708992, 0.432244];
            case 6
                expectedBlerVector = [0.006788, 0.213701, 0.462340, 0.445359];
            case 7
                expectedBlerVector = [0.009277, 0.238621, 0.477600, 0.424872];
            case 8
                expectedBlerVector = [0.011856, 0.201959, 0.713967, 0.304975];
            case 9
                expectedBlerVector = [0.013527, 0.237267, 0.577270, 0.452832];
            case 12
                expectedBlerVector = [0.018851, 0.467728, 0.923449, 0.793764];
            case 14
                expectedBlerVector = [0.041093, 0.222699, 0.921609, 0.762522];
            case 20
                expectedBlerVector = [0.338911, 0.233558, 0.867235, 0.849936];
            otherwise
                error('SINR=%d dB, MCS %d에 대한 테이블이 없습니다.', sinrDb, mcsIndex);
        end

    case 20
        switch mcsIndex
            case 0
                expectedBlerVector = [0.000000, 0.000000, 0.000000, 0.000000];
            case 1
                expectedBlerVector = [0.000000, 0.000000, 0.000000, 0.000000];
            case 2
                expectedBlerVector = [0.000007, 0.000000, 0.000000, 0.000000];
            case 3
                expectedBlerVector = [0.000020, 0.327249, 0.636890, 0.415332];
            case 4
                expectedBlerVector = [0.000064, 0.145433, 0.687026, 0.442063];
            case 5
                expectedBlerVector = [0.000148, 0.109479, 0.501877, 0.388660];
            case 6
                expectedBlerVector = [0.000261, 0.152633, 0.231052, 0.404435];
            case 7
                expectedBlerVector = [0.000557, 0.119758, 0.390880, 0.290148];
            case 8
                expectedBlerVector = [0.001060, 0.070281, 0.651047, 0.208989];
            case 9
                expectedBlerVector = [0.001448, 0.091994, 0.414559, 0.349561];
            case 12
                expectedBlerVector = [0.001278, 0.397253, 0.887322, 0.688052];
            case 14
                expectedBlerVector = [0.002623, 0.205291, 0.917859, 0.702467];
            case 20
                expectedBlerVector = [0.008444, 0.311557, 0.915824, 0.814858];
            otherwise
                error('SINR=%d dB, MCS %d에 대한 테이블이 없습니다.', sinrDb, mcsIndex);
        end

    otherwise
        error('SINR=%d dB에 대한 테이블이 없습니다.', sinrDb);
end
end