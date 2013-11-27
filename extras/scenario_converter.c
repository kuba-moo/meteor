#include "deltaQ.h"

#include <getopt.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define ERROR   -1
#define SUCCESS  0
#define PARAMS_TOTAL        19
#define PROG_NAME           "scenario_converter"

#define BINARY  0
#define TEXT    1

int32_t time_recs;

void
usage()
{
    fprintf(stderr, "scnerio_converter -i input_file -o output_file [-I input_type] [-O output_type]\n");
    fprintf(stderr, "\t -I : input type, text or binary(Default: text)\n");
    fprintf(stderr, "\t -O : output type, text or binary(Default: binary)\n");
}

int32_t
bin2txt(ifile_fd, ofile_fd)
FILE *ifile_fd;
FILE *ofile_fd;
{
    int32_t bin_rec_max_cnt;
    int64_t time_i;
    struct bin_hdr_cls bin_hdr;
    struct bin_time_rec_cls bin_time_rec;
    struct bin_rec_cls *recs = NULL;

    double src_node_x, src_node_y, src_node_z;
    double dst_node_x, dst_node_y, dst_node_z;
    double distance, pr;

    src_node_x = src_node_y = src_node_z = 0.0;
    dst_node_x = dst_node_y = dst_node_z = 0.0;
    distance = pr = 0.0;

    if(io_binary_read_header_from_file(&bin_hdr, ifile_fd) == ERROR) {
        fprintf(stderr, "Aborting on input error (binary header)");
        fclose(ifile_fd);
        exit(1);
    }

    printf("* HEADER INFORMATION:\n");
    io_binary_print_header(&bin_hdr);

    bin_rec_max_cnt = bin_hdr.if_num * (bin_hdr.if_num - 1);
    recs = (struct bin_rec_cls *)calloc(bin_rec_max_cnt, sizeof(struct bin_rec_cls));
    if(recs == NULL) {
        fprintf(stderr, "Cannot allocate memory for records");
        fclose(ifile_fd);
        exit(1);
    }

    fprintf(ofile_fd, "%% Output generated by %s\n", PROG_NAME);
    fprintf(ofile_fd, "%% time from_id from_node_x from_node_y " 
            "from_node_z to_id to_node_x to_node_y to_node_z distance Pr SNR FER "
            "num_retr op_rate bandwidth loss_rate delay jitter\n");

    printf("* RECORD CONTENT:\n");
    for(time_i = 0; time_i < bin_hdr.time_rec_num; time_i++) {
        // read time record
        if(io_binary_read_time_record_from_file(&bin_time_rec, ifile_fd) == ERROR) {
            fprintf(stderr, "Aborting on input error (time record)");
            fclose(ifile_fd);
            exit(1);
        }

        io_binary_print_time_record(&bin_time_rec);
        if(bin_time_rec.record_number > bin_rec_max_cnt) {
            fprintf(stderr, "Number of records exceeds maximum (%d)", bin_rec_max_cnt);
            fclose(ifile_fd);
            exit(1);
        }

        int rec_i;
        if(io_binary_read_records_from_file(recs, bin_time_rec.record_number, ifile_fd) == ERROR) {
            printf("Aborting on input error (records)\n");
            fclose(ifile_fd);
            exit(1);
        }
    
        for(rec_i = 0; rec_i < bin_time_rec.record_number; rec_i++) {
            //io_binary_print_record(&recs[rec_i]);
            fprintf(ofile_fd, "%.4f "
                "%d %.6f %.6f %.6f "
                "%d %.6f %.6f %.6f "
                "%.6f %.6f %d %.6f "
                "%.6f %.6f "
                "%.6f %.6f %.6f\n", 
                bin_time_rec.time, 
                recs[rec_i].from_id, src_node_x, src_node_y, src_node_z,
                recs[rec_i].to_id, dst_node_x, dst_node_y, dst_node_z,
                distance, pr, recs[rec_i].standard, recs[rec_i].frame_error_rate, 
                recs[rec_i].num_retransmissions, recs[rec_i].operating_rate, 
                recs[rec_i].bandwidth, recs[rec_i].delay, recs[rec_i].loss_rate);
        }
    }

    return 0;
}

int32_t
txt2bin(ifile_fd, ofile_fd)
FILE *ifile_fd;
FILE *ofile_fd;
{
    char buf[BUFSIZ];
    int i;
    int32_t src, dst;
    int32_t max_node_num;
    uint32_t rec_i;
    uint32_t rec_num;
    uint32_t rec_size;
    float priv_time = 0.0;
    float time = 0.0;
    float delay;
    float loss_rate;
    float bandwidth;
    float op_rate;
    float num_retx;
    float fer;
    float dummy[PARAMS_TOTAL];

    struct bin_time_rec_cls bin_time_rec;
    struct bin_rec_cls *recs = NULL;
    struct bin_rec_cls *priv_rec = NULL;

    max_node_num = 0;
    time_recs = 0;
    rec_num = 0;

    while(fgets(buf, BUFSIZ, ifile_fd) != NULL) {
        if(sscanf(buf, "%f %d %f %f %f %d %f %f %f %f %f %f " "%f %f %f %f %f %f %f", \
                &time, 
                &src, &dummy[0], &dummy[1], &dummy[2], 
                &dst, &dummy[3], &dummy[4], &dummy[5],  
                &dummy[6], &dummy[7], &dummy[8],
                &dummy[9], &dummy[10], &dummy[11], 
                &bandwidth, &loss_rate, &delay, &dummy[12]) != PARAMS_TOTAL) {
            continue;
        }
        if(src > max_node_num) {
            max_node_num = src;
        }
        if(dst > max_node_num) {
            max_node_num = dst;
        }
        if(priv_time != time) {
            time_recs++;
            priv_time = time;
        }
        fprintf(stderr, "Read Time Records...  %u                 \r", time_recs);
    }
    time_recs++;
    fprintf(stderr, "Read Time Records...  %u                 \n", time_recs);
    max_node_num++;
    fseek(ifile_fd, 0L, SEEK_SET);
    priv_time = 0;

    printf("max node number : %d\n", max_node_num);

    rec_size = max_node_num * max_node_num;
    priv_rec = (struct bin_rec_cls *)calloc(rec_size, sizeof(struct bin_rec_cls));
    recs = (struct bin_rec_cls *)calloc(rec_size, sizeof(struct bin_rec_cls));

    bin_time_rec.time = 0.0;
    bin_time_rec.record_number = max_node_num;

    if(io_binary_write_header_to_file(max_node_num, time_recs, 0, 0, 0, -1, ofile_fd) != 0) {
        fprintf(stderr, "Write Error...\n");
        exit(1);
    }

    rec_num++;
    while(fgets(buf, BUFSIZ, ifile_fd) != NULL) {
        if(sscanf(buf, "%f %d %f %f %f %d %f %f %f %f %f %f " "%f %f %f %f %f %f %f", \
                &time, 
                &src, &dummy[0], &dummy[1], &dummy[2], 
                &dst, &dummy[3], &dummy[4], &dummy[5],  
                &dummy[6], &dummy[7], &dummy[8],
                &fer, &num_retx, &op_rate,
                &bandwidth, &loss_rate, &delay, &dummy[12]) != PARAMS_TOTAL) {
            continue;
        }

        if(priv_time == time) {
            if(fabs(priv_rec[(src * max_node_num) + dst].delay - delay) < FLT_EPSILON
                    && fabs(priv_rec[(src * max_node_num) + dst].loss_rate - loss_rate) < FLT_EPSILON
                    && fabs(priv_rec[(src * max_node_num) + dst].bandwidth - bandwidth) < FLT_EPSILON) {
                continue;
            }
            recs[rec_i].from_id = src;
            recs[rec_i].to_id = dst;
            recs[rec_i].standard = 0;
            recs[rec_i].frame_error_rate = fer;
            recs[rec_i].num_retransmissions = num_retx;
            recs[rec_i].operating_rate = op_rate;
            recs[rec_i].bandwidth = bandwidth;
            recs[rec_i].delay = delay;
            recs[rec_i].loss_rate = loss_rate;
            rec_i++;

            priv_rec[(src * max_node_num) + dst].delay = delay;
            priv_rec[(src * max_node_num) + dst].loss_rate = loss_rate;
            priv_rec[(src * max_node_num) + dst].bandwidth = bandwidth;
        }
        else {
            rec_num++;
            fprintf(stderr, "Write Scenario...  %d/%u                 \r", rec_num, time_recs);
            bin_time_rec.time = priv_time;
            bin_time_rec.record_number = rec_i;
            io_binary_write_time_record_to_file2(&bin_time_rec, ofile_fd);

            if(rec_i != 0) {
                for(i = 0; i < rec_i; i++) {
                    io_binary_write_record_to_file2(&recs[i], ofile_fd);
                }
            }
            else {
                    io_binary_write_record_to_file2(&recs[0], ofile_fd);
            }
            rec_i = 0;
            priv_time = time;

            recs[rec_i].from_id = src;
            recs[rec_i].to_id = dst;
            recs[rec_i].standard = 0;
            recs[rec_i].frame_error_rate = fer;
            recs[rec_i].num_retransmissions = num_retx;
            recs[rec_i].operating_rate = op_rate;
            recs[rec_i].bandwidth = bandwidth;
            recs[rec_i].delay = delay;
            recs[rec_i].loss_rate = loss_rate;
         }
    }
    fprintf(stderr, "Write Scenario...  %d/%u                 \n", rec_num, time_recs);
    bin_time_rec.time = priv_time;
    bin_time_rec.record_number = rec_i;
    io_binary_write_time_record_to_file2(&bin_time_rec, ofile_fd);

    for(i = 0; i <= rec_i; i++) {
        io_binary_write_record_to_file2(&recs[i], ofile_fd);
    }

    free(recs);
    //free(priv_rec);

    return SUCCESS;
}

int32_t
check_type(type)
char *type;
{
    if((strcmp(type, "binary") == 0) || strcmp(type, "bin") == 0) {
        return BINARY;
    }
    else if((strcmp(type, "text") == 0) || strcmp(type, "txt") == 0) {
        return TEXT;
    }

    return ERROR;
}

int
main(argc, argv)
int argc;
char **argv;
{
    FILE *ifile_fd;
    FILE *ofile_fd;
    char c;
    int32_t ifile_type = TEXT;
    int32_t ofile_type = BINARY;

    if(argc < 2) {
        usage();
        exit(1);
    }

    while((c = getopt(argc, argv, "hi:I:o:O:")) != -1) {
        switch(c) {
        case 'h':
            usage();
            exit(0);
        case 'i':
            ifile_fd = fopen(optarg, "r");
            break;
        case 'I':
            ifile_type = check_type(optarg);
            if(ifile_type == -1) {
                fprintf(stderr, "Invalid file type: %s\n", optarg);
                exit(1);
            }
            break;
        case 'o':
            ofile_fd = fopen(optarg, "w");
            if(ofile_fd == NULL) {
                fprintf(stderr, "Cannot open file: %s\n", optarg);
                exit(1);
            }
            break;
        case 'O':
            ofile_type = check_type(optarg);
            if(ofile_type == -1) {
                fprintf(stderr, "Invalid file type: %s\n", optarg);
                exit(1);
            }
            break;
        default: /* '?' */
            usage();
            exit(1);
        }
    }

    if(ifile_fd == NULL || ofile_fd == NULL) {
        usage();
        exit(1);
    }
    
    if(ifile_type == BINARY && ofile_type == TEXT) {
        bin2txt(ifile_fd, ofile_fd);
    }
    else if(ifile_type == TEXT && ofile_type == BINARY) {
        txt2bin(ifile_fd, ofile_fd);
    }

    fclose(ifile_fd);
    fclose(ofile_fd);

    return 0;
}
